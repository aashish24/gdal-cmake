/**********************************************************************
 * $Id: mitab_mapheaderblock.cpp,v 1.9 1999/10/19 16:27:10 warmerda Exp $
 *
 * Name:     mitab_mapheaderblock.cpp
 * Project:  MapInfo TAB Read/Write library
 * Language: C++
 * Purpose:  Implementation of the TABHeaderBlock class used to handle
 *           reading/writing of the .MAP files' header block
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
 * $Log: mitab_mapheaderblock.cpp,v $
 * Revision 1.9  1999/10/19 16:27:10  warmerda
 * Default unitsid to 7 (meters) instead of 0 (miles).
 *
 * Revision 1.8  1999/10/19 06:05:35  daniel
 * Removed obsolete code segments in the coord. conversion functions.
 *
 * Revision 1.7  1999/10/06 13:21:37  daniel
 * Reworked int<->coordsys coords. conversion... hopefully it's OK this time!
 *
 * Revision 1.6  1999/10/01 03:47:38  daniel
 * Better defaults for header fields, and more complete Dump() for debugging
 *
 * Revision 1.5  1999/09/29 04:25:03  daniel
 * Set default scale so that default coord range is +/-1000000.000
 *
 * Revision 1.4  1999/09/26 14:59:36  daniel
 * Implemented write support
 *
 * Revision 1.3  1999/09/21 03:36:33  warmerda
 * slight modification to dump precision
 *
 * Revision 1.2  1999/09/16 02:39:16  daniel
 * Completed read support for most feature types
 *
 * Revision 1.1  1999/07/12 04:18:24  daniel
 * Initial checkin
 *
 **********************************************************************/

#include "mitab.h"

/*---------------------------------------------------------------------
 * Set various constants used in generating the header block.
 *--------------------------------------------------------------------*/
#define HDR_MAGIC_COOKIE        42424242
#define HDR_VERSION_NUMBER      400
#define HDR_BLOCK_SIZE          512

/*---------------------------------------------------------------------
 * The header block starts with an array of map object lenght constants.
 *--------------------------------------------------------------------*/
#define HDR_OBJ_LEN_ARRAY_SIZE   46
static GByte  gabyObjLenArray[ HDR_OBJ_LEN_ARRAY_SIZE  ] = {
            0x00,0x0a,0x0e,0x15,0x0e,0x16,0x1b,0xa2,
            0xa6,0xab,0x1a,0x2a,0x2f,0xa5,0xa9,0xb5,
            0xa7,0xb5,0xd9,0x0f,0x17,0x23,0x13,0x1f,
            0x2b,0x0f,0x17,0x23,0x4f,0x57,0x63,0x9c,
            0xa4,0xa9,0xa0,0xa8,0xad,0xa4,0xa8,0xad,
            0x16,0x1a,0x39,0x0d,0x11,0x37  };



/*=====================================================================
 *                      class TABMAPHeaderBlock
 *====================================================================*/


/**********************************************************************
 *                   TABMAPHeaderBlock::TABMAPHeaderBlock()
 *
 * Constructor.
 **********************************************************************/
TABMAPHeaderBlock::TABMAPHeaderBlock(TABAccess eAccessMode /*= TABRead*/):
    TABRawBinBlock(eAccessMode, TRUE)
{
    int i;

    /*-----------------------------------------------------------------
     * Set acceptable default values for member vars.
     *----------------------------------------------------------------*/
    m_nVersionNumber = 0;
    m_nBlockSize = HDR_BLOCK_SIZE;

    m_dCoordsys2DistUnits = 1.0;
    m_nXMin = -1000000000;
    m_nYMin = -1000000000;
    m_nXMax = 1000000000;
    m_nYMax = 1000000000;

    m_nFirstIndexBlock = 0;
    m_nFirstGarbageBlock = 0;
    m_nFirstToolBlock = 0;

    m_numPointObjects = 0;
    m_numLineObjects = 0;
    m_numRegionObjects = 0;
    m_numTextObjects = 0;
    m_nMaxCoordBufSize = 0;

    m_nDistUnitsCode = 7;       // Meters
    m_nMaxSpIndexDepth = 0;
    m_nCoordPrecision = 3;      // ??? 3 Digits of precision
    m_nCoordOriginQuadrant = 1; // ???
    m_nReflectXAxisCoord = 0;
    m_nMaxObjLenArrayId = HDR_OBJ_LEN_ARRAY_SIZE-1;  // See gabyObjLenArray[]
    m_numPenDefs = 0;
    m_numBrushDefs = 0;
    m_numSymbolDefs = 0;
    m_numFontDefs = 0;
    m_numMapToolBlocks = 0;

    m_sProj.nProjId  = 0;
    m_sProj.nEllipsoidId = 0;
    m_sProj.nUnitsId = 7;
    m_XScale = 1000.0;  // Default coord range (before SetCoordSysBounds()) 
    m_YScale = 1000.0;  // will be [-1000000.000 .. 1000000.000]
    m_XDispl = 0.0;
    m_YDispl = 0.0;

    for(i=0; i<6; i++)
        m_sProj.adProjParams[i] = 0.0;

    m_sProj.dDatumShiftX = 0.0;
    m_sProj.dDatumShiftY = 0.0;
    m_sProj.dDatumShiftZ = 0.0;
    for(i=0; i<5; i++)
        m_sProj.adDatumParams[i] = 0.0;


}

/**********************************************************************
 *                   TABMAPHeaderBlock::~TABMAPHeaderBlock()
 *
 * Destructor.
 **********************************************************************/
TABMAPHeaderBlock::~TABMAPHeaderBlock()
{

}


/**********************************************************************
 *                   TABMAPHeaderBlock::InitBlockFromData()
 *
 * Perform some initialization on the block after its binary data has
 * been set or changed (or loaded from a file).
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABMAPHeaderBlock::InitBlockFromData(GByte *pabyBuf, int nSize, 
                                         GBool bMakeCopy /* = TRUE */,
                                         FILE *fpSrc /* = NULL */, 
                                         int nOffset /* = 0 */)
{
    int i, nStatus;
    GInt32 nMagicCookie;

    /*-----------------------------------------------------------------
     * First of all, we must call the base class' InitBlockFromData()
     *----------------------------------------------------------------*/
    nStatus = TABRawBinBlock::InitBlockFromData(pabyBuf, nSize, bMakeCopy,
                                            fpSrc, nOffset);
    if (nStatus != 0)   
        return nStatus;

    /*-----------------------------------------------------------------
     * Validate block type
     * Header blocks have a magic cookie at byte 0x100
     *----------------------------------------------------------------*/
    GotoByteInBlock(0x100);
    nMagicCookie = ReadInt32();
    if (nMagicCookie != HDR_MAGIC_COOKIE)
    {
        CPLError(CE_Failure, CPLE_FileIO,
              "ReadFromFile(): Invalid Magic Cookie: got %d expected %d",
                 nMagicCookie, HDR_MAGIC_COOKIE);
        CPLFree(m_pabyBuf);
        m_pabyBuf = NULL;
        return -1;
    }

    /*-----------------------------------------------------------------
     * Init member variables
     * Instead of having over 30 get/set methods, we'll make all data 
     * members public and we will initialize them here.  
     * For this reason, this class should be used with care.
     *----------------------------------------------------------------*/
    GotoByteInBlock(0x104);
    m_nVersionNumber = ReadInt16();
    m_nBlockSize = ReadInt16();

    m_dCoordsys2DistUnits = ReadDouble();
    m_nXMin = ReadInt32();
    m_nYMin = ReadInt32();
    m_nXMax = ReadInt32();
    m_nYMax = ReadInt32();

    GotoByteInBlock(0x130);     // Skip 16 unknown bytes 

    m_nFirstIndexBlock = ReadInt32();
    m_nFirstGarbageBlock = ReadInt32();
    m_nFirstToolBlock = ReadInt32();

    m_numPointObjects = ReadInt32();
    m_numLineObjects = ReadInt32();
    m_numRegionObjects = ReadInt32();
    m_numTextObjects = ReadInt32();
    m_nMaxCoordBufSize = ReadInt32();

    GotoByteInBlock(0x15e);     // Skip 14 unknown bytes

    m_nDistUnitsCode = ReadByte();
    m_nMaxSpIndexDepth = ReadByte();
    m_nCoordPrecision = ReadByte();
    m_nCoordOriginQuadrant = ReadByte();
    m_nReflectXAxisCoord = ReadByte();
    m_nMaxObjLenArrayId = ReadByte();    // See gabyObjLenArray[]
    m_numPenDefs = ReadByte();
    m_numBrushDefs = ReadByte();
    m_numSymbolDefs = ReadByte();
    m_numFontDefs = ReadByte();
    m_numMapToolBlocks = ReadInt16();

    GotoByteInBlock(0x16d);     // Skip 3 unknown bytes

    m_sProj.nProjId  = ReadByte();
    m_sProj.nEllipsoidId = ReadByte();
    m_sProj.nUnitsId = ReadByte();
    m_XScale = ReadDouble();
    m_YScale = ReadDouble();
    m_XDispl = ReadDouble();
    m_YDispl = ReadDouble();

    for(i=0; i<6; i++)
        m_sProj.adProjParams[i] = ReadDouble();

    m_sProj.dDatumShiftX = ReadDouble();
    m_sProj.dDatumShiftY = ReadDouble();
    m_sProj.dDatumShiftZ = ReadDouble();
    for(i=0; i<5; i++)
        m_sProj.adDatumParams[i] = ReadDouble();

    return 0;
}


/**********************************************************************
 *                   TABMAPHeaderBlock::Int2Coordsys()
 *
 * Convert from long integer (internal) to coordinates system units
 * as defined in the file's coordsys clause.
 *
 * Note that the false easting/northing and the conversion factor from
 * datum to coordsys units are not included in the calculation.
 *
 * Returns 0 on success, -1 on error.
 **********************************************************************/
int TABMAPHeaderBlock::Int2Coordsys(GInt32 nX, GInt32 nY, 
                                    double &dX, double &dY)
{
    if (m_pabyBuf == NULL)
        return -1;

    // For some obscure reason, some guy decided that it would be 
    // more fun to be able to define our own origin quadrant!

    if (m_nCoordOriginQuadrant==2 || m_nCoordOriginQuadrant==3)
        dX = -1.0 * (nX + m_XDispl) / m_XScale;
    else
        dX = (nX - m_XDispl) / m_XScale;

    if (m_nCoordOriginQuadrant==3 || m_nCoordOriginQuadrant==4)
        dY = -1.0 * (nY + m_YDispl) / m_YScale;
    else
        dY = (nY - m_YDispl) / m_YScale;

//printf("Int2Coordsys: (%d, %d) -> (%.10g, %.10g)\n", nX, nY, dX, dY);

    return 0;
}

/**********************************************************************
 *                   TABMAPHeaderBlock::Coordsys2Int()
 *
 * Convert from coordinates system units as defined in the file's 
 * coordsys clause to long integer (internal) coordinates.
 *
 * Note that the false easting/northing and the conversion factor from
 * datum to coordsys units are not included in the calculation.
 *
 * Returns 0 on success, -1 on error.
 **********************************************************************/
int TABMAPHeaderBlock::Coordsys2Int(double dX, double dY, 
                                    GInt32 &nX, GInt32 &nY)
{
    if (m_pabyBuf == NULL)
        return -1;

    // For some obscure reason, some guy decided that it would be 
    // more fun to be able to define our own origin quadrant!

    if (m_nCoordOriginQuadrant==2 || m_nCoordOriginQuadrant==3)
        nX = (GInt32)(-1.0*dX*m_XScale - m_XDispl);
    else
        nX = (GInt32)(dX*m_XScale + m_XDispl);

    if (m_nCoordOriginQuadrant==3 || m_nCoordOriginQuadrant==4)
        nY = (GInt32)(-1.0*dY*m_YScale - m_YDispl);
    else
        nY = (GInt32)(dY*m_YScale + m_YDispl);

//printf("Coordsys2Int: (%10g, %10g) -> (%d, %d)\n", dX, dY, nX, nY);

    return 0;
}

/**********************************************************************
 *                   TABMAPHeaderBlock::ComprInt2Coordsys()
 *
 * Convert from compressed integer (internal) to coordinates system units
 * as defined in the file's coordsys clause.
 * The difference between long integer and compressed integer coords is
 * that compressed coordinates are scaled displacement relative to an 
 * object centroid.
 *
 * Note that the false easting/northing and the conversion factor from
 * datum to coordsys units are not included in the calculation.
 *
 * Returns 0 on success, -1 on error.
 **********************************************************************/
int TABMAPHeaderBlock::ComprInt2Coordsys(GInt32 nCenterX, GInt32 nCenterY, 
                                         int nDeltaX, int nDeltaY, 
                                         double &dX, double &dY)
{
    if (m_pabyBuf == NULL)
        return -1;

    return Int2Coordsys(nCenterX+nDeltaX, nCenterY+nDeltaY, dX, dY);
}


/**********************************************************************
 *                   TABMAPHeaderBlock::Int2CoordsysDist()
 *
 * Convert a pair of X and Y size (or distance) value from long integer
 * (internal) to coordinates system units as defined in the file's 
 * coordsys clause.
 *
 * The difference with Int2Coordsys() is that this function only applies
 * the scaling factor: it does not apply the displacement.
 *
 * Since the calculations on the X and Y values are independent, either
 * one can be omitted (i.e. passed as 0)
 *
 * Returns 0 on success, -1 on error.
 **********************************************************************/
int TABMAPHeaderBlock::Int2CoordsysDist(GInt32 nX, GInt32 nY, 
                                    double &dX, double &dY)
{
    if (m_pabyBuf == NULL)
        return -1;

    dX = nX / m_XScale;
    dY = nY / m_YScale;

    return 0;
}

/**********************************************************************
 *                   TABMAPHeaderBlock::Coordsys2IntDist()
 *
 * Convert a pair of X and Y size (or distance) values from coordinates
 * system units as defined in the file's coordsys clause to long integer
 * (internal) coordinates.
 *
 * The difference with Coordsys2Int() is that this function only applies
 * the scaling factor: it does not apply the displacement.
 *
 * Since the calculations on the X and Y values are independent, either
 * one can be omitted (i.e. passed as 0)
 *
 * Returns 0 on success, -1 on error.
 **********************************************************************/
int TABMAPHeaderBlock::Coordsys2IntDist(double dX, double dY, 
                                        GInt32 &nX, GInt32 &nY)
{
    if (m_pabyBuf == NULL)
        return -1;

    nX = (GInt32)(dX*m_XScale);
    nY = (GInt32)(dY*m_YScale);

    return 0;
}


/**********************************************************************
 *                   TABMAPHeaderBlock::SetCoordsysBounds()
 *
 * Take projection coordinates bounds of the newly created dataset and
 * compute new values for the X/Y Scales and X/Y displacement.
 *
 * This function must be called after creating a new dataset and before any
 * of the coordinates conversion functions can be used.
 *
 * Returns 0 on success, -1 on error.
 **********************************************************************/
int TABMAPHeaderBlock::SetCoordsysBounds(double dXMin, double dYMin, 
                                         double dXMax, double dYMax)
{
//printf("SetCoordsysBounds(%10g, %10g, %10g, %10g)\n", dXMin, dYMin, dXMax, dYMax);
    /*-----------------------------------------------------------------
     * Check for 0-width or 0-height bounds
     *----------------------------------------------------------------*/
    if (dXMax == dXMin)
    {
        dXMin -= 1.0;
        dXMax += 1.0;
    }

    if (dYMax == dYMin)
    {
        dYMin -= 1.0;
        dYMax += 1.0;
    }

    /*-----------------------------------------------------------------
     * X and Y scales are used to map coordsys coordinates to integer
     * internal coordinates.  We want to find the scale and displacement
     * values that will result in an integer coordinate range of
     * (-1e9, -1e9) - (1e9, 1e9)
     *
     * Note that we ALWAYS generate datasets with the OriginQuadrant = 1
     * so that we avoid reverted X/Y axis complications, etc.
     *----------------------------------------------------------------*/
    m_XScale = 2e9 / (dXMax - dXMin);
    m_YScale = 2e9 / (dYMax - dYMin);

    m_XDispl = -1.0 * m_XScale * (dXMax + dXMin) / 2;
    m_YDispl = -1.0 * m_YScale * (dYMax + dYMin) / 2;

    m_nXMin = -1000000000;
    m_nYMin = -1000000000;
    m_nXMax = 1000000000;
    m_nYMax = 1000000000;

    return 0;
}

/**********************************************************************
 *                   TABMAPHeaderBlock::GetMapObjectSize()
 *
 * Return the size of the object body for the specified object type.
 * The value is looked up in the first 256 bytes of the header.
 **********************************************************************/
int TABMAPHeaderBlock::GetMapObjectSize(int nObjType)
{
    if (m_pabyBuf == NULL)
    {
        CPLError(CE_Failure, CPLE_AssertionFailed,
                 "Block has not been initialized yet!");
        return -1;
    }

    if (nObjType < 0 || nObjType > 255)
    {
        CPLError(CE_Failure, CPLE_IllegalArg,
                 "Invalid object type %d", nObjType);
        return -1;
    }

    // Byte 0x80 is set for objects that have coordinates inside type 3 blocks
    return (m_pabyBuf[nObjType] & 0x7f);
}

/**********************************************************************
 *                   TABMAPHeaderBlock::MapObjectUsesCoordBlock()
 *
 * Return TRUE if the specified map object type has coordinates stored
 * inside type 3 coordinate blocks.
 * The info is looked up in the first 256 bytes of the header.
 **********************************************************************/
GBool TABMAPHeaderBlock::MapObjectUsesCoordBlock(int nObjType)
{
    if (m_pabyBuf == NULL)
    {
        CPLError(CE_Failure, CPLE_AssertionFailed,
                 "Block has not been initialized yet!");
        return FALSE;
    }

    if (nObjType < 0 || nObjType > 255)
    {
        CPLError(CE_Failure, CPLE_IllegalArg,
                 "Invalid object type %d", nObjType);
        return FALSE;
    }

    // Byte 0x80 is set for objects that have coordinates inside type 3 blocks

    return ((m_pabyBuf[nObjType] & 0x80) != 0) ? TRUE: FALSE;
}


/**********************************************************************
 *                   TABMAPHeaderBlock::GetProjInfo()
 *
 * Fill the psProjInfo structure with the projection parameters previously
 * read from this header block.
 *
 * Returns 0 on success, -1 on error.
 **********************************************************************/
int  TABMAPHeaderBlock::GetProjInfo(TABProjInfo *psProjInfo)
{
    if (m_pabyBuf == NULL)
    {
        CPLError(CE_Failure, CPLE_AssertionFailed,
                 "Block has not been initialized yet!");
        return -1;
    }

    if (psProjInfo)
        *psProjInfo = m_sProj;

    return 0;
}

/**********************************************************************
 *                   TABMAPHeaderBlock::SetProjInfo()
 *
 * Set the projection parameters for this dataset.
 *
 * Returns 0 on success, -1 on error.
 **********************************************************************/
int  TABMAPHeaderBlock::SetProjInfo(TABProjInfo *psProjInfo)
{
    if (m_pabyBuf == NULL)
    {
        CPLError(CE_Failure, CPLE_AssertionFailed,
                 "Block has not been initialized yet!");
        return -1;
    }

    if (psProjInfo)
        m_sProj = *psProjInfo;

    return 0;
}


/**********************************************************************
 *                   TABMAPHeaderBlock::CommitToFile()
 *
 * Commit the current state of the binary block to the file to which 
 * it has been previously attached.
 *
 * This method makes sure all values are properly set in the header
 * block buffer and then calls TABRawBinBlock::CommitToFile() to do
 * the actual writing to disk.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABMAPHeaderBlock::CommitToFile()
{
    int i, nStatus = 0;

    if ( m_pabyBuf == NULL || m_nBlockSize != HDR_BLOCK_SIZE )
    {
        CPLError(CE_Failure, CPLE_AssertionFailed, 
        "TABRawBinBlock::CommitToFile(): Block has not been initialized yet!");
        return -1;
    }

    /*-----------------------------------------------------------------
     * Reconstruct header to make sure it is in sync with members variables.
     *----------------------------------------------------------------*/
    GotoByteInBlock(0x000);
    WriteBytes(HDR_OBJ_LEN_ARRAY_SIZE, gabyObjLenArray);
    m_nMaxObjLenArrayId = HDR_OBJ_LEN_ARRAY_SIZE-1;

    GotoByteInBlock(0x100);
    WriteInt32(HDR_MAGIC_COOKIE);
    WriteInt16(HDR_VERSION_NUMBER);
    WriteInt16(HDR_BLOCK_SIZE);

    WriteDouble(m_dCoordsys2DistUnits);
    WriteInt32(m_nXMin);
    WriteInt32(m_nYMin);
    WriteInt32(m_nXMax);
    WriteInt32(m_nYMax);

    WriteZeros(16);     // ???

    WriteInt32(m_nFirstIndexBlock);
    WriteInt32(m_nFirstGarbageBlock);
    WriteInt32(m_nFirstToolBlock);

    WriteInt32(m_numPointObjects);
    WriteInt32(m_numLineObjects);
    WriteInt32(m_numRegionObjects);
    WriteInt32(m_numTextObjects);
    WriteInt32(m_nMaxCoordBufSize);

    WriteZeros(14);     // ???

    WriteByte(m_nDistUnitsCode);
    WriteByte(m_nMaxSpIndexDepth);
    WriteByte(m_nCoordPrecision);
    WriteByte(m_nCoordOriginQuadrant);
    WriteByte(m_nReflectXAxisCoord);
    WriteByte(m_nMaxObjLenArrayId);    // See gabyObjLenArray[]
    WriteByte(m_numPenDefs);
    WriteByte(m_numBrushDefs);
    WriteByte(m_numSymbolDefs);
    WriteByte(m_numFontDefs);
    WriteInt16(m_numMapToolBlocks);

    WriteZeros(3);      // ???

    WriteByte(m_sProj.nProjId);
    WriteByte(m_sProj.nEllipsoidId);
    WriteByte(m_sProj.nUnitsId);
    WriteDouble(m_XScale);
    WriteDouble(m_YScale);
    WriteDouble(m_XDispl);
    WriteDouble(m_YDispl);

    for(i=0; i<6; i++)
        WriteDouble(m_sProj.adProjParams[i]);

    WriteDouble(m_sProj.dDatumShiftX);
    WriteDouble(m_sProj.dDatumShiftY);
    WriteDouble(m_sProj.dDatumShiftZ);
    for(i=0; i<5; i++)
        WriteDouble(m_sProj.adDatumParams[i]);

    /*-----------------------------------------------------------------
     * OK, call the base class to write the block to disk.
     *----------------------------------------------------------------*/
    if (nStatus == 0)
        nStatus = TABRawBinBlock::CommitToFile();

    return nStatus;
}

/**********************************************************************
 *                   TABMAPHeaderBlock::InitNewBlock()
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
int     TABMAPHeaderBlock::InitNewBlock(FILE *fpSrc, int nBlockSize, 
                                        int nFileOffset /* = 0*/)
{
    int i;
    /*-----------------------------------------------------------------
     * Start with the default initialisation
     *----------------------------------------------------------------*/
    if ( TABRawBinBlock::InitNewBlock(fpSrc, nBlockSize, nFileOffset) != 0)
        return -1;

    /*-----------------------------------------------------------------
     * Set acceptable default values for member vars.
     *----------------------------------------------------------------*/
    m_nVersionNumber = 0;
    m_nBlockSize = HDR_BLOCK_SIZE;

    m_dCoordsys2DistUnits = 1.0;
    m_nXMin = -1000000000;
    m_nYMin = -1000000000;
    m_nXMax = 1000000000;
    m_nYMax = 1000000000;

    m_nFirstIndexBlock = 0;
    m_nFirstGarbageBlock = 0;
    m_nFirstToolBlock = 0;

    m_numPointObjects = 0;
    m_numLineObjects = 0;
    m_numRegionObjects = 0;
    m_numTextObjects = 0;
    m_nMaxCoordBufSize = 0;

    m_nDistUnitsCode = 7;       // Meters
    m_nMaxSpIndexDepth = 0;
    m_nCoordPrecision = 3;      // ??? 3 digits of precision
    m_nCoordOriginQuadrant = 1; // ??? N-E quadrant
    m_nReflectXAxisCoord = 0;
    m_nMaxObjLenArrayId = HDR_OBJ_LEN_ARRAY_SIZE-1;  // See gabyObjLenArray[]
    m_numPenDefs = 0;
    m_numBrushDefs = 0;
    m_numSymbolDefs = 0;
    m_numFontDefs = 0;
    m_numMapToolBlocks = 0;

    m_sProj.nProjId  = 0;
    m_sProj.nEllipsoidId = 0;
    m_sProj.nUnitsId = 7;
    m_XScale = 1000.0;  // Default coord range (before SetCoordSysBounds()) 
    m_YScale = 1000.0;  // will be [-1000000.000 .. 1000000.000]
    m_XDispl = 0.0;
    m_YDispl = 0.0;

    for(i=0; i<6; i++)
        m_sProj.adProjParams[i] = 0.0;

    m_sProj.dDatumShiftX = 0.0;
    m_sProj.dDatumShiftY = 0.0;
    m_sProj.dDatumShiftZ = 0.0;
    for(i=0; i<5; i++)
        m_sProj.adDatumParams[i] = 0.0;

    /*-----------------------------------------------------------------
     * And Set the map object length array in the buffer...
     *----------------------------------------------------------------*/
    if (m_eAccess != TABRead)
    {
        GotoByteInBlock(0x000);
        WriteBytes(HDR_OBJ_LEN_ARRAY_SIZE, gabyObjLenArray);
    }

    if (CPLGetLastErrorNo() != 0)
        return -1;

    return 0;
}


/**********************************************************************
 *                   TABMAPHeaderBlock::Dump()
 *
 * Dump block contents... available only in DEBUG mode.
 **********************************************************************/
#ifdef DEBUG

void TABMAPHeaderBlock::Dump(FILE *fpOut /*=NULL*/)
{
    int i;

    if (fpOut == NULL)
        fpOut = stdout;

    fprintf(fpOut, "----- TABMAPHeaderBlock::Dump() -----\n");

    if (m_pabyBuf == NULL)
    {
        fprintf(fpOut, "Block has not been initialized yet.");
    }
    else
    {
        fprintf(fpOut,"Version %d header block.\n", m_nVersionNumber);
        fprintf(fpOut,"  m_nBlockSize          = %d\n", m_nBlockSize);
        fprintf(fpOut,"  m_nFirstIndexBlock    = %d\n", m_nFirstIndexBlock);
        fprintf(fpOut,"  m_nFirstGarbageBlock  = %d\n", m_nFirstGarbageBlock);
        fprintf(fpOut,"  m_nFirstToolBlock     = %d\n", m_nFirstToolBlock);
        fprintf(fpOut,"  m_numPointObjects     = %d\n", m_numPointObjects);
        fprintf(fpOut,"  m_numLineObjects      = %d\n", m_numLineObjects);
        fprintf(fpOut,"  m_numRegionObjects    = %d\n", m_numRegionObjects);
        fprintf(fpOut,"  m_numTextObjects      = %d\n", m_numTextObjects);
        fprintf(fpOut,"  m_nMaxCoordBufSize    = %d\n", m_nMaxCoordBufSize);

        fprintf(fpOut,"\n");
        fprintf(fpOut,"  m_dCoordsys2DistUnits = %g\n", m_dCoordsys2DistUnits);
        fprintf(fpOut,"  m_nXMin               = %d\n", m_nXMin);
        fprintf(fpOut,"  m_nYMin               = %d\n", m_nYMin);
        fprintf(fpOut,"  m_nXMax               = %d\n", m_nXMax);
        fprintf(fpOut,"  m_nYMax               = %d\n", m_nYMax);
        fprintf(fpOut,"  m_XScale              = %g\n", m_XScale);
        fprintf(fpOut,"  m_YScale              = %g\n", m_YScale);
        fprintf(fpOut,"  m_XDispl              = %g\n", m_XDispl);
        fprintf(fpOut,"  m_YDispl              = %g\n", m_YDispl);

        fprintf(fpOut,"\n");
        fprintf(fpOut,"  m_nDistUnistCode      = %d\n", m_nDistUnitsCode);
        fprintf(fpOut,"  m_nMaxSpIndexDepth    = %d\n", m_nMaxSpIndexDepth);
        fprintf(fpOut,"  m_nCoordPrecision     = %d\n", m_nCoordPrecision);
        fprintf(fpOut,"  m_nCoordOriginQuadrant= %d\n",m_nCoordOriginQuadrant);
        fprintf(fpOut,"  m_nReflecXAxisCoord   = %d\n", m_nReflectXAxisCoord);
        fprintf(fpOut,"  m_nMaxObjLenArrayId   = %d\n", m_nMaxObjLenArrayId);
        fprintf(fpOut,"  m_numPenDefs          = %d\n", m_numPenDefs);
        fprintf(fpOut,"  m_numBrushDefs        = %d\n", m_numBrushDefs);
        fprintf(fpOut,"  m_numSymbolDefs       = %d\n", m_numSymbolDefs);
        fprintf(fpOut,"  m_numFontDefs         = %d\n", m_numFontDefs);
        fprintf(fpOut,"  m_numMapToolBlocks    = %d\n", m_numMapToolBlocks);

        fprintf(fpOut,"\n");
        fprintf(fpOut,"  m_sProj.nProjId       = %d\n", (int)m_sProj.nProjId);
        fprintf(fpOut,"  m_sProj.nEllipsoidId  = %d\n", 
                                                    (int)m_sProj.nEllipsoidId);
        fprintf(fpOut,"  m_sProj.nUnitsId      = %d\n", (int)m_sProj.nUnitsId);
        fprintf(fpOut,"  m_sProj.adProjParams  =");
        for(i=0; i<6; i++)
            fprintf(fpOut, " %g",  m_sProj.adProjParams[i]);
        fprintf(fpOut,"\n");

        fprintf(fpOut,"  m_sProj.dDatumShiftX  = %.15g\n", m_sProj.dDatumShiftX);
        fprintf(fpOut,"  m_sProj.dDatumShiftY  = %.15g\n", m_sProj.dDatumShiftY);
        fprintf(fpOut,"  m_sProj.dDatumShiftZ  = %.15g\n", m_sProj.dDatumShiftZ);
        fprintf(fpOut,"  m_sProj.adDatumParams =");
        for(i=0; i<5; i++)
            fprintf(fpOut, " %.15g",  m_sProj.adDatumParams[i]);
        fprintf(fpOut,"\n");

        // Dump array of map object lengths... optional
        if (FALSE)
        {
            fprintf(fpOut, "-- Header bytes 00-FF: Array of map object lenghts --\n");
            for(i=0; i<256; i++)
            {
                fprintf(fpOut, "0x%2.2x", (int)m_pabyBuf[i]);
                if (i != 255)
                    fprintf(fpOut, ",");
                if ((i+1)%16 == 0)
                    fprintf(fpOut, "\n");
            }
        }

    }

    fflush(fpOut);
}

#endif // DEBUG
