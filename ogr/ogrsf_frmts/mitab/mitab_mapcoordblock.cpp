/**********************************************************************
 * $Id: mitab_mapcoordblock.cpp,v 1.6 1999/11/08 04:29:31 daniel Exp $
 *
 * Name:     mitab_mapcoordblock.cpp
 * Project:  MapInfo TAB Read/Write library
 * Language: C++
 * Purpose:  Implementation of the TABMAPCoordBlock class used to handle
 *           reading/writing of the .MAP files' coordinate blocks
 * Author:   Daniel Morissette, danmo@videotron.ca
 *
 **********************************************************************
 * Copyright (c) 1999, Daniel Morissette
 *
 * All rights reserved.  This software may be copied or reproduced, in
 * all or in part, without the prior written consent of its author,
 * Daniel Morissette (danmo@videotron.ca).  However, any material copied
 * or reproduced must bear the original copyright notice (above), this 
 * original paragraph, and the original disclaimer (below).
 * 
 * The entire risk as to the results and performance of the software,
 * supporting text and other information contained in this file
 * (collectively called the "Software") is with the user.  Although 
 * considerable efforts have been used in preparing the Software, the 
 * author does not warrant the accuracy or completeness of the Software.
 * In no event will the author be liable for damages, including loss of
 * profits or consequential damages, arising out of the use of the 
 * Software.
 * 
 **********************************************************************
 *
 * $Log: mitab_mapcoordblock.cpp,v $
 * Revision 1.6  1999/11/08 04:29:31  daniel
 * Fixed problem with compressed coord. offset for regions and multiplines
 *
 * Revision 1.5  1999/10/06 15:19:11  daniel
 * Do not automatically init. curr. feature MBR when block is initialized
 *
 * Revision 1.4  1999/10/06 13:18:55  daniel
 * Fixed uninitialized class members
 *
 * Revision 1.3  1999/09/29 04:25:42  daniel
 * Fixed typo in GetFeatureMBR()
 *
 * Revision 1.2  1999/09/26 14:59:36  daniel
 * Implemented write support
 *
 * Revision 1.1  1999/07/12 04:18:24  daniel
 * Initial checkin
 *
 **********************************************************************/

#include "mitab.h"

/*=====================================================================
 *                      class TABMAPCoordBlock
 *====================================================================*/

#define MAP_COORD_HEADER_SIZE   8

/**********************************************************************
 *                   TABMAPCoordBlock::TABMAPCoordBlock()
 *
 * Constructor.
 **********************************************************************/
TABMAPCoordBlock::TABMAPCoordBlock(TABAccess eAccessMode /*= TABRead*/):
    TABRawBinBlock(eAccessMode, TRUE)
{
    m_nCenterX = m_nCenterY = m_nNextCoordBlock = m_numDataBytes = 0;

    m_numBlocksInChain = 1;  // Current block counts as 1
 
    m_poBlockManagerRef = NULL;

    m_nTotalDataSize = 0;
    m_nFeatureDataSize = 0;

    m_nFeatureXMin = m_nMinX = 1000000000;
    m_nFeatureYMin = m_nMinY = 1000000000;
    m_nFeatureXMax = m_nMaxX = -1000000000;
    m_nFeatureYMax = m_nMaxY = -1000000000;

}

/**********************************************************************
 *                   TABMAPCoordBlock::~TABMAPCoordBlock()
 *
 * Destructor.
 **********************************************************************/
TABMAPCoordBlock::~TABMAPCoordBlock()
{

}


/**********************************************************************
 *                   TABMAPCoordBlock::InitBlockFromData()
 *
 * Perform some initialization on the block after its binary data has
 * been set or changed (or loaded from a file).
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABMAPCoordBlock::InitBlockFromData(GByte *pabyBuf, int nSize, 
                                         GBool bMakeCopy /* = TRUE */,
                                         FILE *fpSrc /* = NULL */, 
                                         int nOffset /* = 0 */)
{
    int nStatus;

    /*-----------------------------------------------------------------
     * First of all, we must call the base class' InitBlockFromData()
     *----------------------------------------------------------------*/
    nStatus = TABRawBinBlock::InitBlockFromData(pabyBuf, nSize, bMakeCopy,
                                            fpSrc, nOffset);
    if (nStatus != 0)   
        return nStatus;

    /*-----------------------------------------------------------------
     * Validate block type
     *----------------------------------------------------------------*/
    if (m_nBlockType != TABMAP_COORD_BLOCK)
    {
        CPLError(CE_Failure, CPLE_FileIO,
                 "InitBlockFromData(): Invalid Block Type: got %d expected %d",
                 m_nBlockType, TABMAP_COORD_BLOCK);
        CPLFree(m_pabyBuf);
        m_pabyBuf = NULL;
        return -1;
    }

    /*-----------------------------------------------------------------
     * Init member variables
     *----------------------------------------------------------------*/
    GotoByteInBlock(0x002);
    m_numDataBytes = ReadInt16();       /* Excluding 8 bytes header */

    m_nNextCoordBlock = ReadInt32();

    /*-----------------------------------------------------------------
     * The read ptr is now located at the beginning of the data part.
     *----------------------------------------------------------------*/
    GotoByteInBlock(MAP_COORD_HEADER_SIZE);

    return 0;
}

/**********************************************************************
 *                   TABMAPCoordBlock::CommitToFile()
 *
 * Commit the current state of the binary block to the file to which 
 * it has been previously attached.
 *
 * This method makes sure all values are properly set in the map object
 * block header and then calls TABRawBinBlock::CommitToFile() to do
 * the actual writing to disk.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABMAPCoordBlock::CommitToFile()
{
    int nStatus = 0;

    if ( m_pabyBuf == NULL )
    {
        CPLError(CE_Failure, CPLE_AssertionFailed, 
                 "CommitToFile(): Block has not been initialized yet!");
        return -1;
    }

    /*-----------------------------------------------------------------
     * Make sure 8 bytes block header is up to date.
     *----------------------------------------------------------------*/
    GotoByteInBlock(0x000);

    WriteInt16(TABMAP_COORD_BLOCK);    // Block type code
    WriteInt16(m_nSizeUsed - MAP_COORD_HEADER_SIZE); // num. bytes used
    WriteInt32(m_nNextCoordBlock);

    nStatus = CPLGetLastErrorNo();

    /*-----------------------------------------------------------------
     * OK, call the base class to write the block to disk.
     *----------------------------------------------------------------*/
    if (nStatus == 0)
        nStatus = TABRawBinBlock::CommitToFile();

    return nStatus;
}

/**********************************************************************
 *                   TABMAPCoordBlock::InitNewBlock()
 *
 * Initialize a newly created block so that it knows to which file it
 * is attached, its block size, etc . and then perform any specific 
 * initialization for this block type, including writing a default 
 * block header, etc. and leave the block ready to receive data.
 *
 * This is an alternative to calling ReadFromFile() or InitBlockFromData()
 * that puts the block in a stable state without loading any initial
 * data in it.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABMAPCoordBlock::InitNewBlock(FILE *fpSrc, int nBlockSize, 
                                        int nFileOffset /* = 0*/)
{
    /*-----------------------------------------------------------------
     * Start with the default initialisation
     *----------------------------------------------------------------*/
    if ( TABRawBinBlock::InitNewBlock(fpSrc, nBlockSize, nFileOffset) != 0)
        return -1;

    /*-----------------------------------------------------------------
     * And then set default values for the block header.
     *----------------------------------------------------------------*/
    m_nNextCoordBlock = 0;
 
    m_nCenterX = m_nCenterY = 0;
    m_numDataBytes = 0;

    // m_nMin/Max are used to keep track of current block MBR
    // FeatureMin/Max should not be reset here since feature coords can
    // be split on several blocks
    m_nMinX = 1000000000;
    m_nMinY = 1000000000;
    m_nMaxX = -1000000000;
    m_nMaxY = -1000000000;

    if (m_eAccess != TABRead)
    {
        GotoByteInBlock(0x000);

        WriteInt16(TABMAP_COORD_BLOCK); // Block type code
        WriteInt16(0);                  // num. bytes used, excluding header
        WriteInt32(0);                  // Pointer to next coord block
    }

    if (CPLGetLastErrorNo() != 0)
        return -1;

    return 0;
}

/**********************************************************************
 *                   TABMAPObjectBlock::SetNextCoordBlock()
 *
 * Set the address (offset from beginning of file) of the coord. block
 * that follows the current one.
 **********************************************************************/
void     TABMAPCoordBlock::SetNextCoordBlock(GInt32 nNextCoordBlockAddress)
{
    m_nNextCoordBlock = nNextCoordBlockAddress;
}


/**********************************************************************
 *                   TABMAPObjectBlock::SetComprCoordOrigin()
 *
 * Set the Compressed integer coordinates space origin to be used when
 * reading compressed coordinates using ReadIntCoord().
 **********************************************************************/
void     TABMAPCoordBlock::SetComprCoordOrigin(GInt32 nX, GInt32 nY)
{
    m_nCenterX = nX;
    m_nCenterY = nY;
}

/**********************************************************************
 *                   TABMAPObjectBlock::ReadIntCoord()
 *
 * Read the next pair of integer coordinates value from the block, and
 * apply the translation relative to the origin of the coord. space
 * previously set using SetComprCoordOrigin() if bCompressed=TRUE.
 *
 * This means that the returned coordinates are always absolute integer
 * coordinates, even when the source coords are in compressed form.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABMAPCoordBlock::ReadIntCoord(GBool bCompressed, 
                                        GInt32 &nX, GInt32 &nY)
{
    if (bCompressed)
    {   
        nX = m_nCenterX + ReadInt16();
        nY = m_nCenterY + ReadInt16();
    }
    else
    {
        nX = ReadInt32();
        nY = ReadInt32();
    }

    if (CPLGetLastErrorNo() != 0)
        return -1;

    return 0;
}

/**********************************************************************
 *                   TABMAPObjectBlock::ReadIntCoords()
 *
 * Read the specified number of pairs of X,Y integer coordinates values
 * from the block, and apply the translation relative to the origin of
 * the coord. space previously set using SetComprCoordOrigin() if 
 * bCompressed=TRUE.
 *
 * This means that the returned coordinates are always absolute integer
 * coordinates, even when the source coords are in compressed form.
 *
 * panXY should point to an array big enough to receive the specified
 * number of coordinates.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABMAPCoordBlock::ReadIntCoords(GBool bCompressed, int numCoordPairs, 
                                        GInt32 *panXY)
{
    int i, numValues = numCoordPairs*2;

    if (bCompressed)
    {   
        for(i=0; i<numValues; i+=2)
        {
            panXY[i]   = m_nCenterX + ReadInt16();
            panXY[i+1] = m_nCenterY + ReadInt16();
            if (CPLGetLastErrorNo() != 0)
                return -1;
        }
    }
    else
    {
        for(i=0; i<numValues; i+=2)
        {
            panXY[i]   = ReadInt32();
            panXY[i+1] = ReadInt32();
            if (CPLGetLastErrorNo() != 0)
                return -1;
        }
    }

    return 0;
}

/**********************************************************************
 *                   TABMAPObjectBlock::ReadCoordSecHdrs()
 *
 * Read a set of coordinate section headers for PLINE MULTIPLE or REGIONs
 * and store the result in the array of structures pasHdrs[].  It is assumed
 * that pasHdrs points to an allocated array of at least numSections 
 * TABMAPCoordSecHdr structures.
 *
 * The function will also set the values of numVerticesTotal to the 
 * total number of coordinates in the object (the sum of all sections 
 * headers read).
 *
 * At the end of the call, this TABMAPCoordBlock object will be located
 * at the beginning of the coordinate data.
 *
 * IMPORTANT: This function makes the assumption that coordinates for all
 *            the sections are grouped together immediately after the
 *            last section header block (i.e. that the coord. data is not
 *            located all over the place).  If it is not the case then
 *            an error will be produced and the code to read region and
 *            multipline objects will have to be updated. 
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABMAPCoordBlock::ReadCoordSecHdrs(GBool bCompressed, int numSections,
                                           TABMAPCoordSecHdr *pasHdrs,
                                           int    &numVerticesTotal)
{
    int i, nTotalHdrSizeUncompressed;


    /*-------------------------------------------------------------
     * Note about header+vertices size vs compressed coordinates:
     * The uncompressed header sections are actually 16 bytes, but the
     * offset calculations are based on prior decompression of the 
     * coordinates.  Our coordinate offset calculations have
     * to take this fact into account.
     *------------------------------------------------------------*/
    nTotalHdrSizeUncompressed = 24 * numSections;

    numVerticesTotal = 0;

    for(i=0; i<numSections; i++)
    {
        /*-------------------------------------------------------------
         * Read the coord. section header blocks
         *------------------------------------------------------------*/
        pasHdrs[i].numVertices = ReadInt16();
        pasHdrs[i].numHoles = ReadInt16();
        ReadIntCoord(bCompressed, pasHdrs[i].nXMin, pasHdrs[i].nYMin);
        ReadIntCoord(bCompressed, pasHdrs[i].nXMax, pasHdrs[i].nYMax);
        pasHdrs[i].nDataOffset = ReadInt32();

        if (CPLGetLastErrorNo() != 0)
            return -1;

        numVerticesTotal += pasHdrs[i].numVertices;


        pasHdrs[i].nVertexOffset = (pasHdrs[i].nDataOffset - 
                                    nTotalHdrSizeUncompressed ) / 8;

    }

    for(i=0; i<numSections; i++)
    {
        /*-------------------------------------------------------------
         * Make sure all coordinates are grouped together
         * (Well... at least check that all the vertex indices are enclosed
         * inside the [0..numVerticesTotal] range.)
         *------------------------------------------------------------*/
        if ( pasHdrs[i].nVertexOffset < 0 || 
             (pasHdrs[i].nVertexOffset +
                           pasHdrs[i].numVertices ) > numVerticesTotal)
        {
            CPLError(CE_Failure, CPLE_AssertionFailed,
                     "Unsupported case or corrupt file: MULTIPLINE/REGION "
                     "object vertices do not appear to be grouped together.");
            return -1;
        }
    }

    return 0;
}

/**********************************************************************
 *                   TABMAPObjectBlock::WriteCoordSecHdrs()
 *
 * Write a set of coordinate section headers for PLINE MULTIPLE or REGIONs.
 * pasHdrs should point to an array of numSections TABMAPCoordSecHdr 
 * structures that have been properly initialized.
 *
 * At the end of the call, this TABMAPCoordBlock object will be ready to
 * receive the coordinate data.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABMAPCoordBlock::WriteCoordSecHdrs(int numSections,
                                            TABMAPCoordSecHdr *pasHdrs)
{
    int i;

    for(i=0; i<numSections; i++)
    {
        /*-------------------------------------------------------------
         * Write the coord. section header blocks
         *------------------------------------------------------------*/
        WriteInt16(pasHdrs[i].numVertices);
        WriteInt16(pasHdrs[i].numHoles);
        WriteIntCoord(pasHdrs[i].nXMin, pasHdrs[i].nYMin);
        WriteIntCoord(pasHdrs[i].nXMax, pasHdrs[i].nYMax);
        WriteInt32(pasHdrs[i].nDataOffset);

        if (CPLGetLastErrorNo() != 0)
            return -1;
    }

    return 0;
}


/**********************************************************************
 *                   TABMAPCoordBlock::WriteIntCoord()
 *
 * Write a pair of integer coordinates values to the current position in the
 * the block.  For now, compressed integer coordinates are NOT supported.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABMAPCoordBlock::WriteIntCoord(GInt32 nX, GInt32 nY,
                                        GBool bUpdateMBR /*=TRUE*/)
{

    if (WriteInt32(nX) != 0 ||
        WriteInt32(nY) != 0 )
    {
        return -1;
    }

    /*-----------------------------------------------------------------
     * Update MBR and block center unless explicitly requested not to do so.
     *----------------------------------------------------------------*/
    if (bUpdateMBR)
    {
        if (nX < m_nMinX)
            m_nMinX = nX;
        if (nX > m_nMaxX)
            m_nMaxX = nX;

        if (nY < m_nMinY)
            m_nMinY = nY;
        if (nY > m_nMaxY)
            m_nMaxY = nY;
    
        m_nCenterX = (m_nMinX + m_nMaxX) /2;
        m_nCenterY = (m_nMinY + m_nMaxY) /2;

        /*-------------------------------------------------------------
         * Also keep track of current feature MBR.
         *------------------------------------------------------------*/
        if (nX < m_nFeatureXMin)
            m_nFeatureXMin = nX;
        if (nX > m_nFeatureXMax)
            m_nFeatureXMax = nX;

        if (nY < m_nFeatureYMin)
            m_nFeatureYMin = nY;
        if (nY > m_nFeatureYMax)
            m_nFeatureYMax = nY;

    }

    return 0;
}

/**********************************************************************
 *                   TABMAPCoordBlock::SetMAPBlockManagerRef()
 *
 * Pass a reference to the block manager object for the file this 
 * block belongs to.  The block manager will be used by this object
 * when it needs to automatically allocate a new block.
 **********************************************************************/
void TABMAPCoordBlock::SetMAPBlockManagerRef(TABMAPBlockManager *poBlockMgr)
{
    m_poBlockManagerRef = poBlockMgr;
};


/**********************************************************************
 *                   TABMAPCoordBlock::ReadBytes()
 *
 * Cover function for TABRawBinBlock::ReadBytes() that will automagically
 * load the next coordinate block in the chain before reading the 
 * requested bytes if we are at the end of the current block and if
 * m_nNextCoordBlock is a valid block.
 *
 * Then the control is passed to TABRawBinBlock::ReadBytes() to finish the
 * work:
 * Copy the number of bytes from the data block's internal buffer to
 * the user's buffer pointed by pabyDstBuf.
 *
 * Passing pabyDstBuf = NULL will only move the read pointer by the
 * specified number of bytes as if the copy had happened... but it 
 * won't crash.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABMAPCoordBlock::ReadBytes(int numBytes, GByte *pabyDstBuf)
{
    int nStatus;

    if (m_pabyBuf && 
        m_nCurPos >= (m_numDataBytes+MAP_COORD_HEADER_SIZE) && 
        m_nNextCoordBlock > 0)
    {
        if ( (nStatus=GotoByteInFile(m_nNextCoordBlock)) != 0)
        {
            // Failed.... an error has already been reported.
            return nStatus;
        }

        GotoByteInBlock(MAP_COORD_HEADER_SIZE); // Move pointer past header
        m_numBlocksInChain++;
    }

    return TABRawBinBlock::ReadBytes(numBytes, pabyDstBuf);
}


/**********************************************************************
 *                   TABMAPCoordBlock::WriteBytes()
 *
 * Cover function for TABRawBinBlock::WriteBytes() that will automagically
 * CommitToFile() the current block and create a new one if we are at 
 * the end of the current block.
 *
 * Then the control is passed to TABRawBinBlock::WriteBytes() to finish the
 * work.
 *
 * Passing pabySrcBuf = NULL will only move the write pointer by the
 * specified number of bytes as if the copy had happened... but it 
 * won't crash.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int  TABMAPCoordBlock::WriteBytes(int nBytesToWrite, GByte *pabySrcBuf)
{
    if (m_eAccess == TABWrite && m_poBlockManagerRef &&
        (m_nBlockSize - m_nCurPos) < nBytesToWrite)
    {
        int nNewBlockOffset = m_poBlockManagerRef->AllocNewBlock();
        SetNextCoordBlock(nNewBlockOffset);

        if (CommitToFile() != 0 ||
            InitNewBlock(m_fp, 512, nNewBlockOffset) != 0)
        {
            // An error message should have already been reported.
            return -1;
        }

        m_numBlocksInChain++;
    }

    if (m_nCurPos >= MAP_COORD_HEADER_SIZE)
    {
        // Keep track of Coordinate data... this means ignore header bytes
        // that could be written.
        m_nTotalDataSize += nBytesToWrite;
        m_nFeatureDataSize += nBytesToWrite;
    }

    return TABRawBinBlock::WriteBytes(nBytesToWrite, pabySrcBuf);
}

/**********************************************************************
 *                   TABMAPCoordBlock::StartNewFeature()
 *
 * Reset all member vars that are used to keep track of data size
 * and MBR for the current feature.  This is info is not needed by
 * the coord blocks themselves, but it helps a lot the callers to
 * have this class take care of that for them.
 *
 * See Also: GetFeatureDataSize() and GetFeatureMBR()
 **********************************************************************/
void TABMAPCoordBlock::StartNewFeature()
{
    m_nFeatureDataSize = 0;

    m_nFeatureXMin = 1000000000;
    m_nFeatureYMin = 1000000000;
    m_nFeatureXMax = -1000000000;
    m_nFeatureYMax = -1000000000;
}

/**********************************************************************
 *                   TABMAPCoordBlock::GetFeatureMBR()
 *
 * Return the MBR of all the coords written using WriteIntCoord() since
 * the last call to StartNewFeature().
 **********************************************************************/
void TABMAPCoordBlock::GetFeatureMBR(GInt32 &nXMin, GInt32 &nYMin, 
                                     GInt32 &nXMax, GInt32 &nYMax)
{
    nXMin = m_nFeatureXMin;
    nYMin = m_nFeatureYMin;
    nXMax = m_nFeatureXMax;
    nYMax = m_nFeatureYMax; 
}


/**********************************************************************
 *                   TABMAPCoordBlock::Dump()
 *
 * Dump block contents... available only in DEBUG mode.
 **********************************************************************/
#ifdef DEBUG

void TABMAPCoordBlock::Dump(FILE *fpOut /*=NULL*/)
{
    if (fpOut == NULL)
        fpOut = stdout;

    fprintf(fpOut, "----- TABMAPCoordBlock::Dump() -----\n");
    if (m_pabyBuf == NULL)
    {
        fprintf(fpOut, "Block has not been initialized yet.");
    }
    else
    {
        fprintf(fpOut,"Coordinate Block (type %d) at offset %d.\n", 
                                                 m_nBlockType, m_nFileOffset);
        fprintf(fpOut,"  m_numDataBytes        = %d\n", m_numDataBytes);
        fprintf(fpOut,"  m_nNextCoordBlock     = %d\n", m_nNextCoordBlock);
    }

    fflush(fpOut);
}

#endif // DEBUG



