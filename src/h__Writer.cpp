/*
Copyright (c) 2004-2021, John Hurst
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*! \file    h__Writer.cpp
    \version $Id$
    \brief   MXF file writer base class
*/

#include "AS_DCP_internal.h"
#include "KLV.h"

using namespace ASDCP;
using namespace ASDCP::MXF;

//
ui32_t
ASDCP::derive_timecode_rate_from_edit_rate(const ASDCP::Rational& edit_rate)
{
  return (ui32_t)floor(0.5 + edit_rate.Quotient());
}

//
// add DMS CryptographicFramework entry to source package
void
ASDCP::AddDmsCrypt(Partition& HeaderPart, SourcePackage& Package,
		   WriterInfo& Descr, const UL& WrappingUL, const Dictionary* Dict)
{
  assert(Dict);
  // Essence Track
  StaticTrack* NewTrack = new StaticTrack(Dict);
  HeaderPart.AddChildObject(NewTrack);
  Package.Tracks.push_back(NewTrack->InstanceUID);
  NewTrack->TrackName = "Descriptive Track";
  NewTrack->TrackID = 3;

  Sequence* Seq = new Sequence(Dict);
  HeaderPart.AddChildObject(Seq);
  NewTrack->Sequence = Seq->InstanceUID;
  Seq->DataDefinition = UL(Dict->ul(MDD_DescriptiveMetaDataDef));

  DMSegment* Segment = new DMSegment(Dict);
  HeaderPart.AddChildObject(Segment);
  Seq->StructuralComponents.push_back(Segment->InstanceUID);
  Segment->EventComment = "AS-DCP KLV Encryption";
  Segment->DataDefinition = UL(Dict->ul(MDD_DescriptiveMetaDataDef));

  CryptographicFramework* CFW = new CryptographicFramework(Dict);
  HeaderPart.AddChildObject(CFW);
  Segment->DMFramework = CFW->InstanceUID;

  CryptographicContext* Context = new CryptographicContext(Dict);
  HeaderPart.AddChildObject(Context);
  CFW->ContextSR = Context->InstanceUID;

  Context->ContextID.Set(Descr.ContextID);
  Context->SourceEssenceContainer = WrappingUL; // ??????
  Context->CipherAlgorithm.Set(Dict->ul(MDD_CipherAlgorithm_AES));
  Context->MICAlgorithm.Set( Descr.UsesHMAC ? Dict->ul(MDD_MICAlgorithm_HMAC_SHA1) : Dict->ul(MDD_MICAlgorithm_NONE) );
  Context->CryptographicKeyID.Set(Descr.CryptographicKeyID);
}

static std::string const rp2057_static_track_label = "SMPTE RP 2057 Generic Stream Text-Based Set";

//
static bool
id_batch_contains(const Array<Kumu::UUID>& batch, const Kumu::UUID& value)
{
  Array<Kumu::UUID>::const_iterator i;
  for ( i = batch.begin(); i != batch.end(); ++i )
    {
      if ( *i == value )
	{
	  return true;
	}
    }
  return false;
}

//
Result_t
ASDCP::AddDmsTrackGenericPartUtf8Text(Kumu::FileWriter& file_writer, MXF::OP1aHeader& header_part,
                      SourcePackage& source_package, MXF::RIP& rip, const Dictionary* Dict,
                      const std::string& trackDescription,  const std::string& dataDescription,
                      std::list<ui64_t*>& durationUpdateList)
{
  Sequence* Sequence_obj = 0;
  InterchangeObject* tmp_iobj = 0;
  std::list<InterchangeObject*> object_list;

  // get the SourcePackage else die
  header_part.GetMDObjectByType(Dict->ul(MDD_SourcePackage), &tmp_iobj);
  SourcePackage *SourcePackage_obj = dynamic_cast<SourcePackage*>(tmp_iobj);
  if ( SourcePackage_obj == 0 )
    {
      DefaultLogSink().Error("MXF Metadata contains no SourcePackage Set.\n");
      return RESULT_FORMAT;
    }

  // find the first StaticTrack object, having the right label, that is ref'd by the source package
  StaticTrack *StaticTrack_obj = 0;
  header_part.GetMDObjectsByType(Dict->ul(MDD_StaticTrack), object_list);
  std::list<InterchangeObject*>::iterator j;
  // start with 2 because there one other track in Material Package: Audio Essence track
  ui32_t newTrackId = 2;
  for ( j = object_list.begin(); j != object_list.end(); ++j )
    {
      StaticTrack_obj = dynamic_cast<StaticTrack*>(*j);
      assert(StaticTrack_obj);
      if ( id_batch_contains(SourcePackage_obj->Tracks, StaticTrack_obj->InstanceUID)
	   && StaticTrack_obj->TrackName.get() == rp2057_static_track_label )
	{
          newTrackId = StaticTrack_obj->TrackID;
          break;
	}
      if (StaticTrack_obj->TrackID >= newTrackId)
      {
          newTrackId = StaticTrack_obj->TrackID + 1;
      }
      StaticTrack_obj = 0;
    }

  // find the Sequence associated with this Track
  if ( StaticTrack_obj )
    {
      object_list.clear();
      header_part.GetMDObjectsByType(Dict->ul(MDD_Sequence), object_list);
      for ( j = object_list.begin(); j != object_list.end(); ++j )
	{
	  Sequence_obj = dynamic_cast<Sequence*>(*j);
	  assert(Sequence_obj);
	  if ( Sequence_obj->InstanceUID == StaticTrack_obj->Sequence )
	    {
	      break;
	    }
	  Sequence_obj = 0;
	}
    }

  if ( Sequence_obj == 0 )
    {
      // this is the first insertion, create the static track
      assert(Dict);
      StaticTrack* static_track = new StaticTrack(Dict);
      header_part.AddChildObject(static_track);
      source_package.Tracks.push_back(static_track->InstanceUID);
      static_track->TrackName = trackDescription;
      static_track->TrackID = newTrackId;

      Sequence_obj = new Sequence(Dict);
      header_part.AddChildObject(Sequence_obj);
      static_track->Sequence = Sequence_obj->InstanceUID;
      Sequence_obj->DataDefinition = UL(Dict->ul(MDD_DescriptiveMetaDataDef));
      Sequence_obj->Duration.set_has_value();
      durationUpdateList.push_back(&Sequence_obj->Duration.get());
      header_part.m_Preface->DMSchemes.push_back(UL(Dict->ul(MDD_MXFTextBasedFramework)));
    }

  assert(Sequence_obj);
  // Create the DM segment and framework packs
  DMSegment* Segment = new DMSegment(Dict);
  header_part.AddChildObject(Segment);
  Sequence_obj->StructuralComponents.push_back(Segment->InstanceUID);
  Segment->EventComment = rp2057_static_track_label;
  Segment->DataDefinition = UL(Dict->ul(MDD_DescriptiveMetaDataDef));
  if (!Segment->Duration.empty())
  {
      durationUpdateList.push_back(&Segment->Duration.get());
  }


  //
  TextBasedDMFramework *dmf_obj = new TextBasedDMFramework(Dict);
  assert(dmf_obj);
  header_part.AddChildObject(dmf_obj);
  Segment->DMFramework = dmf_obj->InstanceUID;
  GenRandomValue(dmf_obj->ObjectRef.get());
  dmf_obj->ObjectRef.set_has_value();

  // Create a new SID on the RIP, located at the current file position
  ui32_t max_sid = 0;
  ASDCP::MXF::RIP::pair_iterator i;
  for ( i = rip.PairArray.begin(); i != rip.PairArray.end(); ++i )
    {
      if ( max_sid < i->BodySID )
	{
	  max_sid = i->BodySID;
	}
    }

  if ( max_sid == 0 )
    {
      DefaultLogSink().Error("Unable to add a GS Partition before the essence container has been established.\n");
      return RESULT_FORMAT;
    }

  rip.PairArray.push_back(RIP::PartitionPair(max_sid + 1, file_writer.TellPosition()));

  // Add new GSTBS linked to DMF
  GenericStreamTextBasedSet *gst_obj = new GenericStreamTextBasedSet(Dict);
  header_part.AddChildObject(gst_obj);
  gst_obj->InstanceUID = dmf_obj->ObjectRef;
  gst_obj->GenericStreamSID = max_sid + 1;
  gst_obj->PayloadSchemeID = UL(Dict->ul(MDD_MXFTextBasedFramework));
  gst_obj->TextDataDescription = dataDescription;
  
  return RESULT_OK;
}

//
ASDCP::h__ASDCPWriter::h__ASDCPWriter(const Dictionary* d) :
  MXF::TrackFileWriter<OP1aHeader>(d), m_BodyPart(d), m_FooterPart(d) {}

ASDCP::h__ASDCPWriter::~h__ASDCPWriter() {}


//
Result_t
ASDCP::h__ASDCPWriter::CreateBodyPart(const MXF::Rational& EditRate, ui32_t BytesPerEditUnit)
{
  assert(m_Dict);
  Result_t result = RESULT_OK;

  // create a body partition if we're writing proper 429-3/OP-Atom
  if ( m_Info.LabelSetType == LS_MXF_SMPTE )
    {
      // Body Partition
      m_BodyPart.EssenceContainers = m_HeaderPart.EssenceContainers;
      m_BodyPart.ThisPartition = m_File.TellPosition();
      m_BodyPart.BodySID = 1;
      UL OPAtomUL(m_Dict->ul(MDD_OPAtom));
      m_BodyPart.OperationalPattern = OPAtomUL;
      m_RIP.PairArray.push_back(RIP::PartitionPair(1, m_BodyPart.ThisPartition)); // Second RIP Entry
      
      UL BodyUL(m_Dict->ul(MDD_ClosedCompleteBodyPartition));
      result = m_BodyPart.WriteToFile(m_File, BodyUL);
    }
  else
    {
      m_HeaderPart.BodySID = 1;
    }

  if ( ASDCP_SUCCESS(result) )
    {
      // Index setup
      Kumu::fpos_t ECoffset = m_File.TellPosition();
      m_FooterPart.IndexSID = 129;

      if ( BytesPerEditUnit == 0 )
	{
	  m_FooterPart.SetIndexParamsVBR(&m_HeaderPart.m_Primer, EditRate, ECoffset);
	}
      else
	{
	  m_FooterPart.SetIndexParamsCBR(&m_HeaderPart.m_Primer, BytesPerEditUnit, EditRate);
	}
    }

  return result;
}

//
Result_t
ASDCP::h__ASDCPWriter::WriteASDCPHeader(const std::string& PackageLabel, const UL& WrappingUL,
					const std::string& TrackName, const UL& EssenceUL, const UL& DataDefinition,
					const MXF::Rational& EditRate, ui32_t TCFrameRate, ui32_t BytesPerEditUnit)
{
  InitHeader(MXFVersion_2004);

  // First RIP Entry
  if ( m_Info.LabelSetType == LS_MXF_SMPTE )  // ERK
    {
      m_RIP.PairArray.push_back(RIP::PartitionPair(0, 0)); // 3-part, no essence in header
    }
  else
    {
      m_RIP.PairArray.push_back(RIP::PartitionPair(1, 0)); // 2-part, essence in header
    }

  // timecode rate and essence rate are the same
  AddSourceClip(EditRate, EditRate, TCFrameRate, TrackName, EssenceUL, DataDefinition, PackageLabel);
  AddEssenceDescriptor(WrappingUL);

#ifdef ASDCP_GCMULTI_PATCH
  UL GenericContainerUL(m_Dict->ul(MDD_GCMulti));
  m_HeaderPart.EssenceContainers.push_back(GenericContainerUL);
#endif

  Result_t result = m_HeaderPart.WriteToFile(m_File, m_HeaderSize);

  if ( KM_SUCCESS(result) )
    result = CreateBodyPart(EditRate, BytesPerEditUnit);

  return result;
}

//
Result_t
ASDCP::h__ASDCPWriter::WriteEKLVPacket(const ASDCP::FrameBuffer& FrameBuf,const byte_t* EssenceUL,
				       const ui32_t& MinEssenceElementBerLength,	       
				       AESEncContext* Ctx, HMACContext* HMAC)
{
  return Write_EKLV_Packet(m_File, *m_Dict, m_HeaderPart, m_Info, m_CtFrameBuf, m_FramesWritten,
			   m_StreamOffset, FrameBuf, EssenceUL, MinEssenceElementBerLength,
			   Ctx, HMAC);
}

// standard method of writing the header and footer of a completed MXF file
//
Result_t
ASDCP::h__ASDCPWriter::WriteASDCPFooter()
{
  // update all Duration properties
  DurationElementList_t::iterator dli = m_DurationUpdateList.begin();

  for (; dli != m_DurationUpdateList.end(); ++dli )
    {
      **dli = m_FramesWritten;
    }

  m_EssenceDescriptor->ContainerDuration = m_FramesWritten;
  m_FooterPart.PreviousPartition = m_RIP.PairArray.back().ByteOffset;

  Kumu::fpos_t here = m_File.TellPosition();
  m_RIP.PairArray.push_back(RIP::PartitionPair(0, here)); // Last RIP Entry
  m_HeaderPart.FooterPartition = here;

  assert(m_Dict);
  // re-label the header partition, set the footer
  UL OPAtomUL(m_Dict->ul(MDD_OPAtom));
  m_HeaderPart.OperationalPattern = OPAtomUL;
  m_HeaderPart.m_Preface->OperationalPattern = OPAtomUL;
  m_FooterPart.OperationalPattern = OPAtomUL;

  m_FooterPart.EssenceContainers = m_HeaderPart.EssenceContainers;
  m_FooterPart.FooterPartition = here;
  m_FooterPart.ThisPartition = here;

  Result_t result = m_FooterPart.WriteToFile(m_File, m_FramesWritten);

  if ( ASDCP_SUCCESS(result) )
    result = m_RIP.WriteToFile(m_File);

  if ( ASDCP_SUCCESS(result) )
    result = m_File.Seek(0);

  if ( ASDCP_SUCCESS(result) )
    result = m_HeaderPart.WriteToFile(m_File, m_HeaderSize);

  m_File.Close();
  return result;
}


//------------------------------------------------------------------------------------------
//


// standard method of writing a plaintext or encrypted frame
Result_t
ASDCP::Write_EKLV_Packet(Kumu::FileWriter& File, const ASDCP::Dictionary& Dict, const MXF::OP1aHeader&,
			 const ASDCP::WriterInfo& Info, ASDCP::FrameBuffer& CtFrameBuf, ui32_t& FramesWritten,
			 ui64_t & StreamOffset, const ASDCP::FrameBuffer& FrameBuf, const byte_t* EssenceUL,
			 const ui32_t& MinEssenceElementBerLength,
			 AESEncContext* Ctx, HMACContext* HMAC)
{
  Result_t result = RESULT_OK;
  IntegrityPack IntPack;

  byte_t overhead[128];
  Kumu::MemIOWriter Overhead(overhead, 128);
  // We declare HMACOverhead and its buffer in the outer scope, even though it is not used on
  // unencrypted content: the reason is that File.Writev(const byte_t* buf, ui32_t buf_len) doesn't
  // write data right away but saves a pointer on the buffer. And we write all the buffers at the end
  // when calling File.writev().
  // Declaring the buffer variable in an inner scope means the buffer will go out of scope
  // before the data it contains has been actually written, which means its content could be
  // overwritten/get corrupted.
  byte_t hmoverhead[512];
  Kumu::MemIOWriter HMACOverhead(hmoverhead, 512);

  if ( FrameBuf.Size() == 0 )
    {
      DefaultLogSink().Error("Cannot write empty frame buffer\n");
      return RESULT_EMPTY_FB;
    }

  if ( Info.EncryptedEssence )
    {
    #ifndef HAVE_OPENSSL
      return RESULT_CRYPT_CTX;
    #else
      if ( ! Ctx )
	return RESULT_CRYPT_CTX;

      if ( Info.UsesHMAC && ! HMAC )
	return RESULT_HMAC_CTX;

      if ( FrameBuf.PlaintextOffset() > FrameBuf.Size() )
	return RESULT_LARGE_PTO;

      // encrypt the essence data (create encrypted source value)
      result = EncryptFrameBuffer(FrameBuf, CtFrameBuf, Ctx);

      // create HMAC
      if ( ASDCP_SUCCESS(result) && Info.UsesHMAC )
      	result = IntPack.CalcValues(CtFrameBuf, Info.AssetUUID, FramesWritten + 1, HMAC);

      if ( ASDCP_SUCCESS(result) )
	{ // write UL
	  Overhead.WriteRaw(Dict.ul(MDD_CryptEssence), SMPTE_UL_LENGTH);

	  // construct encrypted triplet header
	  ui32_t ETLength = klv_cryptinfo_size + CtFrameBuf.Size();
	  ui32_t essence_element_BER_length = MinEssenceElementBerLength;

	  if ( Info.UsesHMAC )
	    ETLength += klv_intpack_size;
	  else
	    ETLength += (MXF_BER_LENGTH * 3); // for empty intpack

	  if ( ETLength > 0x00ffffff ) // Need BER integer longer than MXF_BER_LENGTH bytes
	    {
	      essence_element_BER_length = Kumu::get_BER_length_for_value(ETLength);

	      // the packet is longer by the difference in expected vs. actual BER length
	      ETLength += essence_element_BER_length - MXF_BER_LENGTH;

	      if ( essence_element_BER_length == 0 )
		result = RESULT_KLV_CODING;
	    }

	  if ( ASDCP_SUCCESS(result) )
	    {
	      if ( ! ( Overhead.WriteBER(ETLength, essence_element_BER_length)                      // write encrypted triplet length
		       && Overhead.WriteBER(UUIDlen, MXF_BER_LENGTH)                // write ContextID length
		       && Overhead.WriteRaw(Info.ContextID, UUIDlen)              // write ContextID
		       && Overhead.WriteBER(sizeof(ui64_t), MXF_BER_LENGTH)         // write PlaintextOffset length
		       && Overhead.WriteUi64BE(FrameBuf.PlaintextOffset())          // write PlaintextOffset
		       && Overhead.WriteBER(SMPTE_UL_LENGTH, MXF_BER_LENGTH)        // write essence UL length
		       && Overhead.WriteRaw((byte_t*)EssenceUL, SMPTE_UL_LENGTH)    // write the essence UL
		       && Overhead.WriteBER(sizeof(ui64_t), MXF_BER_LENGTH)         // write SourceLength length
		       && Overhead.WriteUi64BE(FrameBuf.Size())                     // write SourceLength
		       && Overhead.WriteBER(CtFrameBuf.Size(), essence_element_BER_length) ) )    // write ESV length
		{
		  result = RESULT_KLV_CODING;
		}
	    }

	  if ( ASDCP_SUCCESS(result) )
	    result = File.Writev(Overhead.Data(), Overhead.Length());
	}

      if ( ASDCP_SUCCESS(result) )
	{
	  StreamOffset += Overhead.Length();
	  // write encrypted source value
	  result = File.Writev((byte_t*)CtFrameBuf.RoData(), CtFrameBuf.Size());
	}

      if ( ASDCP_SUCCESS(result) )
	{
	  StreamOffset += CtFrameBuf.Size();

	  // write the HMAC
	  if ( Info.UsesHMAC )
	    {
	      HMACOverhead.WriteRaw(IntPack.Data, klv_intpack_size);
	    }
	  else
	    { // we still need the var-pack length values if the intpack is empty
	      for ( ui32_t i = 0; i < 3 ; i++ )
		HMACOverhead.WriteBER(0, MXF_BER_LENGTH);
	    }

	  // write HMAC
	  result = File.Writev(HMACOverhead.Data(), HMACOverhead.Length());
	  StreamOffset += HMACOverhead.Length();
	}
#endif //HAVE_OPENSSL
    }
  else
    {
      ui32_t essence_element_BER_length = MinEssenceElementBerLength;

      if ( FrameBuf.Size() > 0x00ffffff ) // Need BER integer longer than MXF_BER_LENGTH bytes
	{
	  essence_element_BER_length = Kumu::get_BER_length_for_value(FrameBuf.Size());

	  if ( essence_element_BER_length == 0 )
	    result = RESULT_KLV_CODING;
	}

      Overhead.WriteRaw((byte_t*)EssenceUL, SMPTE_UL_LENGTH);
      Overhead.WriteBER(FrameBuf.Size(), essence_element_BER_length);

      if ( ASDCP_SUCCESS(result) )
	result = File.Writev(Overhead.Data(), Overhead.Length());
 
      if ( ASDCP_SUCCESS(result) )
	result = File.Writev((byte_t*)FrameBuf.RoData(), FrameBuf.Size());

      if ( ASDCP_SUCCESS(result) )
	StreamOffset += Overhead.Length() + FrameBuf.Size();
    }

  if ( ASDCP_SUCCESS(result) )
    result = File.Writev();

  return result;
}

//
// end h__Writer.cpp
//
