/*
Copyright (c) 2004-2006, John Hurst
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
#include "MemIO.h"
#include "Timecode.h"
#include "KLV.h"
#include "MDD.h"
#include <assert.h>

using namespace ASDCP;
using namespace ASDCP::MXF;


ASDCP::h__Writer::h__Writer() : m_FramesWritten(0), m_StreamOffset(0)
{
}

ASDCP::h__Writer::~h__Writer()
{
}

// standard method of writing the header of a new MXF file
Result_t
ASDCP::h__Writer::WriteMXFHeader(EssenceType_t EssenceType, ASDCP::Rational& EditRate,
			  ui32_t TCFrameRate, ui32_t BytesPerEditUnit)
{
#if 0
  // write the stream metadata
  m_Metadata = new Metadata();
  assert(m_Metadata);
  assert(m_Metadata->m_Object);

  if ( m_Info.EncryptedEssence )
    {
      UL DMSUL(CryptoFrameworkUL_Data);
      m_Metadata->AddDMScheme(DMSUL);
    }

  // Set the OP label
  // If we are writing OP-Atom we write the header as OP1a initially as another process
  // may try to read the file before it is complete and then it will NOT be a valid OP-Atom file
  m_Metadata->SetOP(OP1aUL);

  // Build the Material Package
  // DRAGONS: We should really try and determine the UMID type rather than cop-out!
  UMID PackageUMID;
  PackageUMID.MakeUMID(0x0d); // mixed type

  m_MaterialPackage = m_Metadata->AddMaterialPackage("AS-DCP Material Package", PackageUMID);
  m_Metadata->SetPrimaryPackage(m_MaterialPackage);	// This will be overwritten for OP-Atom
  
  TrackPtr MPTimecodeTrack = m_MaterialPackage->AddTimecodeTrack(EditRate);
  m_MPTimecode = MPTimecodeTrack->AddTimecodeComponent(TCFrameRate, 0, 0);

  TrackPtr FPTimecodeTrack = 0;
  mxflib::UUID assetUUID(m_Info.AssetUUID);

  UMID EssenceUMID;
  
  switch ( EssenceType )
    {
    case ESS_MPEG2_VES:
      PackageUMID.MakeUMID(0x0f, assetUUID);
      m_FilePackage = m_Metadata->AddFilePackage(1, MPEG_PACKAGE_LABEL, PackageUMID);
      m_MPTrack = m_MaterialPackage->AddPictureTrack(EditRate);
      m_FPTrack = m_FilePackage->AddPictureTrack(0, EditRate);
      break;
	  
    case ESS_JPEG_2000:
      PackageUMID.MakeUMID(0x0f, assetUUID);
      m_FilePackage = m_Metadata->AddFilePackage(1, JP2K_PACKAGE_LABEL, PackageUMID);
      m_MPTrack = m_MaterialPackage->AddPictureTrack(EditRate);
      m_FPTrack = m_FilePackage->AddPictureTrack(0, EditRate);
      break;
	  
    case ESS_PCM_24b_48k:
      PackageUMID.MakeUMID(0x0f, assetUUID);
      m_FilePackage = m_Metadata->AddFilePackage(1, PCM_PACKAGE_LABEL, PackageUMID);
      m_MPTrack = m_MaterialPackage->AddSoundTrack(EditRate);
      m_FPTrack = m_FilePackage->AddSoundTrack(0, EditRate);
      break;

    default: return RESULT_RAW_ESS;
    }

  // Add an essence element
  FPTimecodeTrack = m_FilePackage->AddTimecodeTrack(EditRate);
  m_FPTimecode = FPTimecodeTrack->AddTimecodeComponent(TCFrameRate, 0/* NDF */,
						       tc_to_frames(TCFrameRate, 1, 0, 0, 0) );
  
  // Add a single Component to this Track of the Material Package
  m_MPClip = m_MPTrack->AddSourceClip();
  
  // Add a single Component to this Track of the File Package
  m_FPClip = m_FPTrack->AddSourceClip();
  const byte_t* SourceEssenceContainerLabel = 0;

  // Frame wrapping
  if ( m_Info.EncryptedEssence )
    {
      switch ( EssenceType )
	{
	case ESS_MPEG2_VES:
	  SourceEssenceContainerLabel = WrappingUL_Data_MPEG2_VES;
          break;
	  
	case ESS_JPEG_2000:
	  SourceEssenceContainerLabel = WrappingUL_Data_JPEG_2000;
          break;
	    
	case ESS_PCM_24b_48k:
	  SourceEssenceContainerLabel = WrappingUL_Data_PCM_24b_48k;
          break;
	  
	default:
	  return RESULT_RAW_ESS;
	}
    }

  mem_ptr<UL> WrappingUL;
  switch ( EssenceType )
    {
    case ESS_MPEG2_VES:
      WrappingUL = new UL(WrappingUL_Data_MPEG2_VES); // memchk TESTED
      break;
        
    case ESS_JPEG_2000:
      WrappingUL = new UL(WrappingUL_Data_JPEG_2000); // memchk TESTED
      break;
        
    case ESS_PCM_24b_48k:
      WrappingUL = new UL(WrappingUL_Data_PCM_24b_48k); // memchk TESTED
      break;
      
    default:
      return RESULT_RAW_ESS;
    }
  assert(!WrappingUL.empty());
  m_EssenceDescriptor->SetValue("EssenceContainer", DataChunk(klv_key_size, WrappingUL->GetValue()));
  
  // Write a File Descriptor only on the internally ref'ed Track 
  m_EssenceDescriptor->SetUint("LinkedTrackID", m_FPTrack->GetUint("TrackID"));
  m_FilePackage->AddChild("Descriptor")->MakeLink(*m_EssenceDescriptor);

  UL CryptEssenceUL(WrappingUL_Data_Crypt);

  if ( m_Info.EncryptedEssence )
    {
      m_Metadata->AddEssenceType(CryptEssenceUL);
    }
  else
    {
      UL GCUL(GCMulti_Data);
      m_Metadata->AddEssenceType(GCUL);
      m_Metadata->AddEssenceType(*WrappingUL);
    }

  // Link the MP to the FP
  m_MPClip->MakeLink(m_FPTrack, 0);

  //
  // ** Write out the header **
  //

  m_HeaderPart = new Partition("OpenHeader");
  assert(m_HeaderPart);
  m_HeaderPart->SetKAG(1);			// Everything else can stay at default
  m_HeaderPart->SetUint("BodySID", 1);
  
  m_HeaderPart->AddMetadata(m_Metadata);

  // Build an Ident set describing us and link into the metadata
  MDObject* Ident = new MDObject("Identification");
  assert(Ident);
  Ident->SetString("CompanyName", m_Info.CompanyName);
  Ident->SetString("ProductName", m_Info.ProductName);
  Ident->SetString("VersionString", m_Info.ProductVersion);
  UUID ProductUID(m_Info.ProductUUID);
  Ident->SetValue("ProductUID", DataChunk(UUIDlen, ProductUID.GetValue()));

  // TODO: get Oliver to show me how this works
  //      Ident->SetString("ToolkitVersion", ?);

  m_Metadata->UpdateGenerations(*Ident);

  if ( m_Info.EncryptedEssence )
    AddDMScrypt(m_FilePackage, m_Info, SourceEssenceContainerLabel);

  // Write the header partition
  m_File->WritePartition(*m_HeaderPart, HeaderPadding);
      
  // set up the index
  switch ( EssenceType )
    {
    case ESS_MPEG2_VES:
    case ESS_JPEG_2000:
      m_IndexMan = new IndexManager(0, 0);
      m_IndexMan->SetPosTableIndex(0, -1);
      break;

    case ESS_PCM_24b_48k:
      m_IndexMan = new IndexManager(0, BytesPerEditUnit);
      break;

    case ESS_UNKNOWN:
      return RESULT_INIT;
    }

  m_IndexMan->SetBodySID(1);
  m_IndexMan->SetIndexSID(129);
  m_IndexMan->SetEditRate(EditRate);
#endif

  return RESULT_OK;
}

// standard method of writing a plaintext or encrypted frame
Result_t
ASDCP::h__Writer::WriteEKLVPacket(const ASDCP::FrameBuffer& FrameBuf, const byte_t* EssenceUL,
				  AESEncContext* Ctx, HMACContext* HMAC)
{
  Result_t result;
  IntegrityPack IntPack;

  byte_t overhead[128];
  MemIOWriter Overhead(overhead, 128);

  if ( FrameBuf.Size() == 0 )
    {
      DefaultLogSink().Error("Cannot write empty frame buffer\n");
      return RESULT_EMPTY_FB;
    }

  if ( m_Info.EncryptedEssence )
    {
      if ( ! Ctx )
	return RESULT_CRYPT_CTX;

      if ( m_Info.UsesHMAC && ! HMAC )
	return RESULT_HMAC_CTX;

      if ( FrameBuf.PlaintextOffset() > FrameBuf.Size() )
	return RESULT_LARGE_PTO;

      // encrypt the essence data (create encrypted source value)
      result = EncryptFrameBuffer(FrameBuf, m_CtFrameBuf, Ctx);

      // create HMAC
      if ( ASDCP_SUCCESS(result) && m_Info.UsesHMAC )
      	result = IntPack.CalcValues(m_CtFrameBuf, m_Info.AssetUUID, m_FramesWritten + 1, HMAC);

      if ( ASDCP_SUCCESS(result) )
	{ // write UL
	  Overhead.WriteRaw((byte_t*)CryptEssenceUL_Data, klv_key_size);

	  // construct encrypted triplet header
	  ui32_t ETLength = klv_cryptinfo_size + m_CtFrameBuf.Size();

	  if ( m_Info.UsesHMAC )
	    ETLength += klv_intpack_size;
	  else
	    ETLength += (klv_length_size * 3); // for empty intpack

	  Overhead.WriteBER(ETLength, klv_length_size);                  // write encrypted triplet length
	  Overhead.WriteBER(UUIDlen, klv_length_size);                   // write ContextID length
	  Overhead.WriteRaw(m_Info.ContextID, UUIDlen);                  // write ContextID
	  Overhead.WriteBER(sizeof(ui64_t), klv_length_size);            // write PlaintextOffset length
	  Overhead.WriteUi64BE(FrameBuf.PlaintextOffset());              // write PlaintextOffset
	  Overhead.WriteBER(klv_key_size, klv_length_size);              // write essence UL length
	  Overhead.WriteRaw((byte_t*)EssenceUL, klv_key_size);           // write the essence UL
	  Overhead.WriteBER(sizeof(ui64_t), klv_length_size);            // write SourceLength length
	  Overhead.WriteUi64BE(FrameBuf.Size());                         // write SourceLength
	  Overhead.WriteBER(m_CtFrameBuf.Size(), klv_length_size);       // write ESV length

	  result = m_File.Writev(Overhead.Data(), Overhead.Size());
	}

      if ( ASDCP_SUCCESS(result) )
	{
	  m_StreamOffset += Overhead.Size();
	  // write encrypted source value
	  result = m_File.Writev((byte_t*)m_CtFrameBuf.RoData(), m_CtFrameBuf.Size());
	}

      if ( ASDCP_SUCCESS(result) )
	{
	  m_StreamOffset += m_CtFrameBuf.Size();

	  byte_t hmoverhead[512];
	  MemIOWriter HMACOverhead(hmoverhead, 512);

	  // write the HMAC
	  if ( m_Info.UsesHMAC )
	    {
	      HMACOverhead.WriteRaw(IntPack.Data, klv_intpack_size);
	    }
	  else
	    { // we still need the var-pack length values if the intpack is empty
	      for ( ui32_t i = 0; i < 3 ; i++ )
		HMACOverhead.WriteBER(0, klv_length_size);
	    }

	  // write HMAC
	  result = m_File.Writev(HMACOverhead.Data(), HMACOverhead.Size());
	  m_StreamOffset += HMACOverhead.Size();
	}
    }
  else
    {
      Overhead.WriteRaw((byte_t*)EssenceUL, klv_key_size);
      Overhead.WriteBER(FrameBuf.Size(), klv_length_size);
      result = m_File.Writev(Overhead.Data(), Overhead.Size());
 
      if ( ASDCP_SUCCESS(result) )
	result = m_File.Writev((byte_t*)FrameBuf.RoData(), FrameBuf.Size());

      if ( ASDCP_SUCCESS(result) )
	m_StreamOffset += Overhead.Size() + FrameBuf.Size();
    }

  if ( ASDCP_SUCCESS(result) )
    result = m_File.Writev();

  return result;
}


// standard method of writing the header and footer of a completed MXF file
//
Result_t
ASDCP::h__Writer::WriteMXFFooter(EssenceType_t EssenceType)
{
#if 0
  // write the index
  DataChunk IndexChunk;
  ui32_t IndexSID = 0;

  // Find all essence container data sets so we can update "IndexSID"
  MDObjectListPtr ECDataSets = 0;
  MDObject* Ptr = (*m_Metadata)["ContentStorage"];
  if ( Ptr )
    Ptr = Ptr->GetLink();
  if ( Ptr )
    Ptr = (*Ptr)["EssenceContainerData"];
  if ( Ptr )
    ECDataSets = Ptr->ChildList("EssenceContainer");

  // ** Handle full index tables next **
  // ***********************************
      
  // Make an index table containing all available entries
  IndexTablePtr Index = m_IndexMan->MakeIndex();
  m_IndexMan->AddEntriesToIndex(Index);
      
  // Write the index table
  Index->WriteIndex(IndexChunk);
				
  // Record the IndexSID for when the index is written
  IndexSID = Index->IndexSID;

  // Update IndexSID in essence container data set
  MDObjectList::iterator ECD_it = ECDataSets->begin();
  while(ECD_it != ECDataSets->end())
    {
      if((*ECD_it)->GetLink())
	{
	  if((*ECD_it)->GetLink()->GetUint("BodySID") == m_IndexMan->GetBodySID())
	    {
	      (*ECD_it)->GetLink()->SetUint("IndexSID", m_IndexMan->GetIndexSID());
	      break;
	    }
	}

      ECD_it++;
    }

  // If we are writing OP-Atom this is the first place we can claim it
  m_Metadata->SetOP(OPAtomUL);
  
  // Set top-level file package correctly for OP-Atom
  m_Metadata->SetPrimaryPackage(m_FilePackage);
  
  m_Metadata->SetTime();
  m_MPTimecode->SetDuration(m_FramesWritten);

  m_MPClip->SetDuration(m_FramesWritten);
  m_FPTimecode->SetDuration(m_FramesWritten);
  m_FPClip->SetDuration(m_FramesWritten);
  m_EssenceDescriptor->SetInt64("ContainerDuration", m_FramesWritten);

  // Turn the header or body partition into a footer
  m_HeaderPart->ChangeType("CompleteFooter");
  m_HeaderPart->SetUint("IndexSID",  IndexSID);

  // Make sure any new sets are linked in
  m_HeaderPart->UpdateMetadata(m_Metadata);

  // Actually write the footer
  m_File.WritePartitionWithIndex(*m_HeaderPart, &IndexChunk, false);

  // Add a RIP
  m_File.WriteRIP();

  //
  // ** Update the header ** 
  //
  // For generalized OPs update the value of "FooterPartition" in the header pack
  // For OP-Atom re-write the entire header
  //
  ASDCP::fpos_t FooterPos = m_HeaderPart->GetUint64("FooterPartition");
  m_File.Seek(0);

  m_HeaderPart->ChangeType("ClosedCompleteHeader");
  m_HeaderPart->SetUint64("FooterPartition", FooterPos);
  m_HeaderPart->SetUint64("BodySID", 1);

  m_File.ReWritePartition(*m_HeaderPart);
  m_File.Close();
#endif
  return RESULT_OK;
}

//
// end h__Writer.cpp
//
