/******************************************************************************
 * $Id$
 *
 * Project:  Microstation DGN Access Library
 * Purpose:  DGN Access Library element reading code.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2000, Avenza Systems Inc, http://www.avenza.com/
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * $Log$
 * Revision 1.28  2002/05/30 19:24:38  warmerda
 * add partial support for tag type 5
 *
 * Revision 1.27  2002/04/22 20:42:40  warmerda
 * fixed problem with tag set ids > 255
 *
 * Revision 1.26  2002/04/08 21:25:59  warmerda
 * removed tagDef->id = iTag+1 assumption
 *
 * Revision 1.25  2002/03/14 21:40:03  warmerda
 * expose DGNLoadRawElement, add max_element_count in psDGN
 *
 * Revision 1.24  2002/03/12 17:11:31  warmerda
 * ensure tag elements are indexed
 *
 * Revision 1.23  2002/03/12 17:07:26  warmerda
 * added tagset and tag value element support
 *
 * Revision 1.22  2002/02/22 22:17:42  warmerda
 * Ensure that components of complex chain/shapes are spatially selected
 * based on the decision made for their owner (header).
 *
 * Revision 1.21  2002/02/06 22:39:08  warmerda
 * dont filter out non-spatial features with spatial filter
 *
 * Revision 1.20  2002/02/06 20:32:33  warmerda
 * handle improbably large elements
 *
 * Revision 1.19  2002/01/21 20:53:33  warmerda
 * a bunch of reorganization to support spatial filtering
 *
 * Revision 1.18  2002/01/15 06:38:51  warmerda
 * moved some functions to dgnhelp.cpp
 *
 * Revision 1.17  2001/12/19 15:29:56  warmerda
 * added preliminary cell header support
 *
 * Revision 1.16  2001/11/09 14:55:29  warmerda
 * fixed 3D support for arcs
 *
 * Revision 1.15  2001/09/27 14:28:44  warmerda
 * first hack at 3D support
 *
 * Revision 1.14  2001/08/28 21:27:07  warmerda
 * added prototype multi-byte character support
 *
 * Revision 1.13  2001/08/21 03:01:39  warmerda
 * added raw_data support
 *
 * Revision 1.12  2001/07/18 04:55:16  warmerda
 * added CPL_CSVID
 *
 * Revision 1.11  2001/06/25 15:07:51  warmerda
 * Added support for DGNElemComplexHeader
 * Don't include elements with the complex bit (such as shared cell definition
 * elements) in extents computation for fear they are in a different coord sys.
 *
 * Revision 1.10  2001/03/07 13:56:44  warmerda
 * updated copyright to be held by Avenza Systems
 *
 * Revision 1.9  2001/03/07 13:52:15  warmerda
 * Don't include deleted elements in the total extents.
 * Capture attribute data.
 *
 * Revision 1.7  2001/02/02 22:20:29  warmerda
 * compute text height/width properly
 *
 * Revision 1.6  2001/01/17 16:07:33  warmerda
 * ensure that TCB and ColorTable side effects occur on indexing pass too
 *
 * Revision 1.5  2001/01/16 18:12:52  warmerda
 * Added arc support, DGNLookupColor
 *
 * Revision 1.4  2001/01/10 16:13:45  warmerda
 * added docs and extents api
 *
 * Revision 1.3  2000/12/28 21:28:59  warmerda
 * added element index support
 *
 * Revision 1.2  2000/12/14 17:10:57  warmerda
 * implemented TCB, Ellipse, TEXT
 *
 * Revision 1.1  2000/11/28 19:03:47  warmerda
 * New
 *
 */

#include "dgnlibp.h"

CPL_CVSID("$Id$");

static DGNElemCore *DGNParseTCB( DGNInfo * );
static DGNElemCore *DGNParseColorTable( DGNInfo * );
static DGNElemCore *DGNParseTagSet( DGNInfo * );

/************************************************************************/
/*                           DGNGotoElement()                           */
/************************************************************************/

/**
 * Seek to indicated element.
 *
 * Changes what element will be read on the next call to DGNReadElement(). 
 * Note that this function requires and index, and one will be built if
 * not already available.
 *
 * @param hDGN the file to affect.
 * @param element_id the element to seek to.  These values are sequentially
 * ordered starting at zero for the first element.
 * 
 * @return returns TRUE on success or FALSE on failure. 
 */

int DGNGotoElement( DGNHandle hDGN, int element_id )

{
    DGNInfo	*psDGN = (DGNInfo *) hDGN;

    DGNBuildIndex( psDGN );

    if( element_id < 0 || element_id >= psDGN->element_count )
        return FALSE;

    if( VSIFSeek( psDGN->fp, psDGN->element_index[element_id].offset, 
                  SEEK_SET ) != 0 )
        return FALSE;

    psDGN->next_element_id = element_id;
    psDGN->in_complex_group = FALSE;

    return TRUE;
}

/************************************************************************/
/*                         DGNLoadRawElement()                          */
/************************************************************************/

int DGNLoadRawElement( DGNInfo *psDGN, int *pnType, int *pnLevel )

{
/* -------------------------------------------------------------------- */
/*      Read the first four bytes to get the level, type, and word      */
/*      count.                                                          */
/* -------------------------------------------------------------------- */
    int		nType, nWords, nLevel;

    if( VSIFRead( psDGN->abyElem, 1, 4, psDGN->fp ) != 4 )
        return FALSE;

    /* Is this an 0xFFFF endof file marker? */
    if( psDGN->abyElem[0] == 0xff && psDGN->abyElem[1] == 0xff )
        return FALSE;

    nWords = psDGN->abyElem[2] + psDGN->abyElem[3]*256;
    nType = psDGN->abyElem[1] & 0x7f;
    nLevel = psDGN->abyElem[0] & 0x3f;

/* -------------------------------------------------------------------- */
/*      Read the rest of the element data into the working buffer.      */
/* -------------------------------------------------------------------- */
    CPLAssert( nWords * 2 + 4 <= (int) sizeof(psDGN->abyElem) );

    if( (int) VSIFRead( psDGN->abyElem + 4, 2, nWords, psDGN->fp ) != nWords )
        return FALSE;

    psDGN->nElemBytes = nWords * 2 + 4;

    psDGN->next_element_id++;

/* -------------------------------------------------------------------- */
/*      Return requested info.                                          */
/* -------------------------------------------------------------------- */
    if( pnType != NULL )
        *pnType = nType;
    
    if( pnLevel != NULL )
        *pnLevel = nLevel;
    
    return TRUE;
}

/************************************************************************/
/*                        DGNGetElementExtents()                        */
/*                                                                      */
/*      Returns FALSE if the element type does not have reconisable     */
/*      element extents, other TRUE and the extents will be updated.    */
/*                                                                      */
/*      It is assumed the raw element data has been loaded into the     */
/*      working area by DGNLoadRawElement().                            */
/************************************************************************/

static int 
DGNGetElementExtents( DGNInfo *psDGN, int nType, 
                      GUInt32 *pnXMin, GUInt32 *pnYMin, GUInt32 *pnZMin, 
                      GUInt32 *pnXMax, GUInt32 *pnYMax, GUInt32 *pnZMax )

{
    switch( nType )
    {
      case DGNT_LINE:
      case DGNT_LINE_STRING:
      case DGNT_SHAPE:
      case DGNT_CURVE:
      case DGNT_BSPLINE:
      case DGNT_ELLIPSE:
      case DGNT_ARC:
      case DGNT_TEXT:
      case DGNT_COMPLEX_CHAIN_HEADER:
      case DGNT_COMPLEX_SHAPE_HEADER:
        *pnXMin = DGN_INT32( psDGN->abyElem + 4 );
        *pnYMin = DGN_INT32( psDGN->abyElem + 8 );
        if( pnZMin != NULL )
            *pnZMin = DGN_INT32( psDGN->abyElem + 12 );

        *pnXMax = DGN_INT32( psDGN->abyElem + 16 );
        *pnYMax = DGN_INT32( psDGN->abyElem + 20 );
        if( pnZMax != NULL )
            *pnZMax = DGN_INT32( psDGN->abyElem + 24 );
        return TRUE;

      default:
        return FALSE;
    }
}

/************************************************************************/
/*                         DGNProcessElement()                          */
/*                                                                      */
/*      Assumes the raw element data has already been loaded, and       */
/*      tries to convert it into an element structure.                  */
/************************************************************************/

static DGNElemCore *DGNProcessElement( DGNInfo *psDGN, int nType, int nLevel )

{
    DGNElemCore *psElement = NULL;

/* -------------------------------------------------------------------- */
/*      Handle based on element type.                                   */
/* -------------------------------------------------------------------- */
    switch( nType )
    {
      case DGNT_CELL_HEADER:
      {
          DGNElemCellHeader *psCell;

          psCell = (DGNElemCellHeader *) 
              CPLCalloc(sizeof(DGNElemCellHeader),1);
          psElement = (DGNElemCore *) psCell;
          psElement->stype = DGNST_CELL_HEADER;
          DGNParseCore( psDGN, psElement );

          psCell->totlength = psDGN->abyElem[36] + psDGN->abyElem[37] * 256;

          DGNRad50ToAscii( psDGN->abyElem[38] + psDGN->abyElem[39] * 256, 
                           psCell->name + 0 );
          DGNRad50ToAscii( psDGN->abyElem[40] + psDGN->abyElem[41] * 256, 
                           psCell->name + 3 );

          psCell->cclass = psDGN->abyElem[42] + psDGN->abyElem[43] * 256;
          psCell->levels[0] = psDGN->abyElem[44] + psDGN->abyElem[45] * 256;
          psCell->levels[1] = psDGN->abyElem[46] + psDGN->abyElem[47] * 256;
          psCell->levels[2] = psDGN->abyElem[48] + psDGN->abyElem[49] * 256;
          psCell->levels[3] = psDGN->abyElem[50] + psDGN->abyElem[51] * 256;
          psCell->core.color = psDGN->abyElem[35];

          if( psDGN->dimension == 2 )
          {
              psCell->rnglow.x = DGN_INT32( psDGN->abyElem + 52 );
              psCell->rnglow.y = DGN_INT32( psDGN->abyElem + 56 );
              psCell->rnghigh.x = DGN_INT32( psDGN->abyElem + 60 );
              psCell->rnghigh.y = DGN_INT32( psDGN->abyElem + 64 );

              psCell->origin.x = DGN_INT32( psDGN->abyElem + 84 );
              psCell->origin.y = DGN_INT32( psDGN->abyElem + 88 );

              {
              double a, b, c, d, a2, c2;
              a = DGN_INT32( psDGN->abyElem + 68 );
              b = DGN_INT32( psDGN->abyElem + 72 );
              c = DGN_INT32( psDGN->abyElem + 76 );
              d = DGN_INT32( psDGN->abyElem + 80 );
              a2 = a * a;
              c2 = c * c;
              
              psCell->xscale = sqrt(a2 + c2) / 214748;
              psCell->yscale = sqrt(b*b + d*d) / 214748;
              psCell->rotation = acos(a / sqrt(a2 + c2));
              if (b <= 0)
                  psCell->rotation = psCell->rotation * 180 / PI;
              else
                  psCell->rotation = 360 - psCell->rotation * 180 / PI;
              }
          }
          else
          {
              psCell->rnglow.x = DGN_INT32( psDGN->abyElem + 52 );
              psCell->rnglow.y = DGN_INT32( psDGN->abyElem + 56 );
              psCell->rnglow.z = DGN_INT32( psDGN->abyElem + 60 );
              psCell->rnghigh.x = DGN_INT32( psDGN->abyElem + 64 );
              psCell->rnghigh.y = DGN_INT32( psDGN->abyElem + 68 );
              psCell->rnghigh.z = DGN_INT32( psDGN->abyElem + 72 );

              psCell->origin.x = DGN_INT32( psDGN->abyElem + 112 );
              psCell->origin.y = DGN_INT32( psDGN->abyElem + 116 );
              psCell->origin.z = DGN_INT32( psDGN->abyElem + 120 );
          }

          DGNTransformPoint( psDGN, &(psCell->rnglow) );
          DGNTransformPoint( psDGN, &(psCell->rnghigh) );
          DGNTransformPoint( psDGN, &(psCell->origin) );
      }
      break;

      case DGNT_CELL_LIBRARY:
      {
          DGNElemCellLibrary *psCell;
          int                 iWord;

          psCell = (DGNElemCellLibrary *) 
              CPLCalloc(sizeof(DGNElemCellLibrary),1);
          psElement = (DGNElemCore *) psCell;
          psElement->stype = DGNST_CELL_LIBRARY;
          DGNParseCore( psDGN, psElement );

          DGNRad50ToAscii( psDGN->abyElem[32] + psDGN->abyElem[33] * 256, 
                           psCell->name + 0 );
          DGNRad50ToAscii( psDGN->abyElem[34] + psDGN->abyElem[35] * 256, 
                           psCell->name + 3 );

          psElement->properties = psDGN->abyElem[38] 
              + psDGN->abyElem[39] * 256;

          psCell->dispsymb = psDGN->abyElem[40] + psDGN->abyElem[41] * 256;

          psCell->cclass = psDGN->abyElem[42] + psDGN->abyElem[43] * 256;
          psCell->levels[0] = psDGN->abyElem[44] + psDGN->abyElem[45] * 256;
          psCell->levels[1] = psDGN->abyElem[46] + psDGN->abyElem[47] * 256;
          psCell->levels[2] = psDGN->abyElem[48] + psDGN->abyElem[49] * 256;
          psCell->levels[3] = psDGN->abyElem[50] + psDGN->abyElem[51] * 256;

          psCell->numwords = psDGN->abyElem[36] + psDGN->abyElem[37] * 256;

          memset( psCell->description, 0, sizeof(psCell->description) );
          
          for( iWord = 0; iWord < 9; iWord++ )
          {
              int iOffset = 52 + iWord * 2;

              DGNRad50ToAscii( psDGN->abyElem[iOffset] 
                               + psDGN->abyElem[iOffset+1] * 256, 
                               psCell->description + iWord * 3 );
          }
      }
      break;

      case DGNT_LINE:
      {
          DGNElemMultiPoint *psLine;

          psLine = (DGNElemMultiPoint *) 
              CPLCalloc(sizeof(DGNElemMultiPoint),1);
          psElement = (DGNElemCore *) psLine;
          psElement->stype = DGNST_MULTIPOINT;
          DGNParseCore( psDGN, psElement );

          psLine->num_vertices = 2;
          if( psDGN->dimension == 2 )
          {
              psLine->vertices[0].x = DGN_INT32( psDGN->abyElem + 36 );
              psLine->vertices[0].y = DGN_INT32( psDGN->abyElem + 40 );
              psLine->vertices[1].x = DGN_INT32( psDGN->abyElem + 44 );
              psLine->vertices[1].y = DGN_INT32( psDGN->abyElem + 48 );
          }
          else
          {
              psLine->vertices[0].x = DGN_INT32( psDGN->abyElem + 36 );
              psLine->vertices[0].y = DGN_INT32( psDGN->abyElem + 40 );
              psLine->vertices[0].z = DGN_INT32( psDGN->abyElem + 44 );
              psLine->vertices[1].x = DGN_INT32( psDGN->abyElem + 48 );
              psLine->vertices[1].y = DGN_INT32( psDGN->abyElem + 52 );
              psLine->vertices[1].z = DGN_INT32( psDGN->abyElem + 56 );
          }
          DGNTransformPoint( psDGN, psLine->vertices + 0 );
          DGNTransformPoint( psDGN, psLine->vertices + 1 );
      }
      break;

      case DGNT_LINE_STRING:
      case DGNT_SHAPE:
      case DGNT_CURVE:
      case DGNT_BSPLINE:
      {
          DGNElemMultiPoint *psLine;
          int                i, count;
          int		     pntsize = psDGN->dimension * 4;

          count = psDGN->abyElem[36] + psDGN->abyElem[37]*256;
          psLine = (DGNElemMultiPoint *) 
              CPLCalloc(sizeof(DGNElemMultiPoint)+(count-2)*sizeof(DGNPoint),1);
          psElement = (DGNElemCore *) psLine;
          psElement->stype = DGNST_MULTIPOINT;
          DGNParseCore( psDGN, psElement );

          if( psDGN->nElemBytes < 38 + count * pntsize )
          {
              CPLError( CE_Warning, CPLE_AppDefined, 
                        "Trimming multipoint vertices to %d from %d because\n"
                        "element is short.\n", 
                        (psDGN->nElemBytes - 38) / pntsize,
                        count );
              count = (psDGN->nElemBytes - 38) / pntsize;
          }
          psLine->num_vertices = count;
          for( i = 0; i < psLine->num_vertices; i++ )
          {
              psLine->vertices[i].x = 
                  DGN_INT32( psDGN->abyElem + 38 + i*pntsize );
              psLine->vertices[i].y = 
                  DGN_INT32( psDGN->abyElem + 42 + i*pntsize );
              if( psDGN->dimension == 3 )
                  psLine->vertices[i].z = 
                      DGN_INT32( psDGN->abyElem + 46 + i*pntsize );

              DGNTransformPoint( psDGN, psLine->vertices + i );
          }
      }
      break;

      case DGNT_GROUP_DATA:
        if( nLevel == DGN_GDL_COLOR_TABLE )
        {
            psElement = DGNParseColorTable( psDGN );
        }
        else
        {
            psElement = (DGNElemCore *) CPLCalloc(sizeof(DGNElemCore),1);
            psElement->stype = DGNST_CORE;
            DGNParseCore( psDGN, psElement );
        }
        break;

      case DGNT_ELLIPSE:
      {
          DGNElemArc *psEllipse;

          psEllipse = (DGNElemArc *) CPLCalloc(sizeof(DGNElemArc),1);
          psElement = (DGNElemCore *) psEllipse;
          psElement->stype = DGNST_ARC;
          DGNParseCore( psDGN, psElement );

          memcpy( &(psEllipse->primary_axis), psDGN->abyElem + 36, 8 );
          DGN2IEEEDouble( &(psEllipse->primary_axis) );
          psEllipse->primary_axis *= psDGN->scale;

          memcpy( &(psEllipse->secondary_axis), psDGN->abyElem + 44, 8 );
          DGN2IEEEDouble( &(psEllipse->secondary_axis) );
          psEllipse->secondary_axis *= psDGN->scale;

          if( psDGN->dimension == 2 )
          {
              psEllipse->rotation = DGN_INT32( psDGN->abyElem + 52 );
              psEllipse->rotation = psEllipse->rotation / 360000.0;
              
              memcpy( &(psEllipse->origin.x), psDGN->abyElem + 56, 8 );
              DGN2IEEEDouble( &(psEllipse->origin.x) );
              
              memcpy( &(psEllipse->origin.y), psDGN->abyElem + 64, 8 );
              DGN2IEEEDouble( &(psEllipse->origin.y) );
          }
          else
          {
              /* leave quaternion for later */

              memcpy( &(psEllipse->origin.x), psDGN->abyElem + 68, 8 );
              DGN2IEEEDouble( &(psEllipse->origin.x) );
              
              memcpy( &(psEllipse->origin.y), psDGN->abyElem + 76, 8 );
              DGN2IEEEDouble( &(psEllipse->origin.y) );
              
              memcpy( &(psEllipse->origin.z), psDGN->abyElem + 84, 8 );
              DGN2IEEEDouble( &(psEllipse->origin.z) );
          }

          DGNTransformPoint( psDGN, &(psEllipse->origin) );

          psEllipse->startang = 0.0;
          psEllipse->sweepang = 360.0;
      }
      break;

      case DGNT_ARC:
      {
          DGNElemArc *psEllipse;
          GInt32     nSweepVal;

          psEllipse = (DGNElemArc *) CPLCalloc(sizeof(DGNElemArc),1);
          psElement = (DGNElemCore *) psEllipse;
          psElement->stype = DGNST_ARC;
          DGNParseCore( psDGN, psElement );

          psEllipse->startang = DGN_INT32( psDGN->abyElem + 36 );
          psEllipse->startang = psEllipse->startang / 360000.0;
          if( psDGN->abyElem[41] & 0x80 )
          {
              psDGN->abyElem[41] &= 0x7f;
              nSweepVal = -1 * DGN_INT32( psDGN->abyElem + 40 );
          }
          else
              nSweepVal = DGN_INT32( psDGN->abyElem + 40 );

          if( nSweepVal == 0 )
              psEllipse->sweepang = 360.0;
          else
              psEllipse->sweepang = nSweepVal / 360000.0;
          
          memcpy( &(psEllipse->primary_axis), psDGN->abyElem + 44, 8 );
          DGN2IEEEDouble( &(psEllipse->primary_axis) );
          psEllipse->primary_axis *= psDGN->scale;

          memcpy( &(psEllipse->secondary_axis), psDGN->abyElem + 52, 8 );
          DGN2IEEEDouble( &(psEllipse->secondary_axis) );
          psEllipse->secondary_axis *= psDGN->scale;
          
          if( psDGN->dimension == 2 )
          {
              psEllipse->rotation = DGN_INT32( psDGN->abyElem + 60 );
              psEllipse->rotation = psEllipse->rotation / 360000.0;
          
              memcpy( &(psEllipse->origin.x), psDGN->abyElem + 64, 8 );
              DGN2IEEEDouble( &(psEllipse->origin.x) );
              
              memcpy( &(psEllipse->origin.y), psDGN->abyElem + 72, 8 );
              DGN2IEEEDouble( &(psEllipse->origin.y) );
          }
          else 
          {
              /* for now we don't try to handle quaternion */
              psEllipse->rotation = 0;
          
              memcpy( &(psEllipse->origin.x), psDGN->abyElem + 76, 8 );
              DGN2IEEEDouble( &(psEllipse->origin.x) );
              
              memcpy( &(psEllipse->origin.y), psDGN->abyElem + 84, 8 );
              DGN2IEEEDouble( &(psEllipse->origin.y) );

              memcpy( &(psEllipse->origin.z), psDGN->abyElem + 92, 8 );
              DGN2IEEEDouble( &(psEllipse->origin.z) );
          }

          DGNTransformPoint( psDGN, &(psEllipse->origin) );

      }
      break;

      case DGNT_TEXT:
      {
          DGNElemText *psText;
          int	      num_chars, text_off;

          if( psDGN->dimension == 2 )
              num_chars = psDGN->abyElem[58];
          else
              num_chars = psDGN->abyElem[74];

          psText = (DGNElemText *) CPLCalloc(sizeof(DGNElemText)+num_chars,1);
          psElement = (DGNElemCore *) psText;
          psElement->stype = DGNST_TEXT;
          DGNParseCore( psDGN, psElement );

          psText->font_id = psDGN->abyElem[36];
          psText->justification = psDGN->abyElem[37];
          psText->length_mult = (DGN_INT32( psDGN->abyElem + 38 ))
              * psDGN->scale * 6.0 / 1000.0;
          psText->height_mult = (DGN_INT32( psDGN->abyElem + 42 ))
              * psDGN->scale * 6.0 / 1000.0;

          if( psDGN->dimension == 2 )
          {
              psText->rotation = DGN_INT32( psDGN->abyElem + 46 );
              psText->rotation = psText->rotation / 360000.0;

              psText->origin.x = DGN_INT32( psDGN->abyElem + 50 );
              psText->origin.y = DGN_INT32( psDGN->abyElem + 54 );
              text_off = 60;
          }
          else
          {
              /* leave quaternion for later */

              psText->origin.x = DGN_INT32( psDGN->abyElem + 62 );
              psText->origin.y = DGN_INT32( psDGN->abyElem + 66 );
              psText->origin.z = DGN_INT32( psDGN->abyElem + 70 );
              text_off = 76;
          }

          DGNTransformPoint( psDGN, &(psText->origin) );

          /* experimental multibyte support from Ason Kang (hiska@netian.com)*/
          if (*(psDGN->abyElem + text_off) == 0xFF 
              && *(psDGN->abyElem + text_off + 1) == 0xFD) 
          {
              int n=0;
              for (int i=0;i<num_chars/2-1;i++) {
                  unsigned short w;
                  memcpy(&w,psDGN->abyElem + text_off + 2 + i*2 ,2);
                  w = CPL_LSBWORD16(w);
                  if (w<256) { // if alpa-numeric code area : Normal character 
                      *(psText->string + n)     = w & 0xFF; 
                      n++; // skip 1 byte;
                  }
                  else { // if extend code area : 2 byte Korean character 
                      *(psText->string + n)     = w >> 8;   // hi
                      *(psText->string + n + 1) = w & 0xFF; // lo
                      n+=2; // 2 byte
                  }
              }
              psText->string[n] = '\0'; // terminate C string
          }
          else
          {
              memcpy( psText->string, psDGN->abyElem + text_off, num_chars );
              psText->string[num_chars] = '\0';
          }
      }
      break;

      case DGNT_TCB:
        psElement = DGNParseTCB( psDGN );
        break;

      case DGNT_COMPLEX_CHAIN_HEADER:
      case DGNT_COMPLEX_SHAPE_HEADER:
      {
          DGNElemComplexHeader *psHdr;

          psHdr = (DGNElemComplexHeader *) 
              CPLCalloc(sizeof(DGNElemComplexHeader),1);
          psElement = (DGNElemCore *) psHdr;
          psElement->stype = DGNST_COMPLEX_HEADER;
          DGNParseCore( psDGN, psElement );

          psHdr->totlength = psDGN->abyElem[36] + psDGN->abyElem[37] * 256;
          psHdr->numelems = psDGN->abyElem[38] + psDGN->abyElem[39] * 256;
      }
      break;

      case DGNT_TAG_VALUE:
      {
          DGNElemTagValue *psTag;

          psTag = (DGNElemTagValue *) 
              CPLCalloc(sizeof(DGNElemTagValue),1);
          psElement = (DGNElemCore *) psTag;
          psElement->stype = DGNST_TAG_VALUE;
          DGNParseCore( psDGN, psElement );

          psTag->tagType = psDGN->abyElem[74] + psDGN->abyElem[75] * 256;
          memcpy( &(psTag->tagSet), psDGN->abyElem + 68, 4 );
          psTag->tagSet = CPL_LSBWORD32(psTag->tagSet);
          psTag->tagIndex = psDGN->abyElem[72] + psDGN->abyElem[73] * 256;
          psTag->tagLength = psDGN->abyElem[150] + psDGN->abyElem[151] * 256;

          if( psTag->tagType == 1 )
          {
              psTag->tagValue.string = 
                  CPLStrdup( (char *) psDGN->abyElem + 154 );
          }
          else if( psTag->tagType == 3 )
          {
              memcpy( &(psTag->tagValue.integer), 
                      psDGN->abyElem + 154, 4 );
              psTag->tagValue.integer = 
                  CPL_LSBWORD32( psTag->tagValue.integer );
          }
          else if( psTag->tagType == 4 )
          {
              memcpy( &(psTag->tagValue.real), 
                      psDGN->abyElem + 154, 8 );
              DGN2IEEEDouble( &(psTag->tagValue.real) );
          }
      }
      break;

      case DGNT_APPLICATION_ELEM:
        if( nLevel == 24 )
        {
            psElement = DGNParseTagSet( psDGN );
        }
        else
        {
            psElement = (DGNElemCore *) CPLCalloc(sizeof(DGNElemCore),1);
            psElement->stype = DGNST_CORE;
            DGNParseCore( psDGN, psElement );
        }
        break;

      default:
      {
          psElement = (DGNElemCore *) CPLCalloc(sizeof(DGNElemCore),1);
          psElement->stype = DGNST_CORE;
          DGNParseCore( psDGN, psElement );
      }
      break;
    }

/* -------------------------------------------------------------------- */
/*      If the element structure type is "core" or if we are running    */
/*      in "capture all" mode, record the complete binary image of      */
/*      the element.                                                    */
/* -------------------------------------------------------------------- */
    if( psElement->stype == DGNST_CORE 
        || (psDGN->options & DGNO_CAPTURE_RAW_DATA) )
    {
        psElement->raw_bytes = psDGN->nElemBytes;
        psElement->raw_data = (unsigned char *)CPLMalloc(psElement->raw_bytes);

        memcpy( psElement->raw_data, psDGN->abyElem, psElement->raw_bytes );
    }

/* -------------------------------------------------------------------- */
/*      Collect some additional generic information.                    */
/* -------------------------------------------------------------------- */
    psElement->element_id = psDGN->next_element_id - 1;

    psElement->offset = VSIFTell( psDGN->fp ) - psDGN->nElemBytes;
    psElement->size = psDGN->nElemBytes;

    return psElement;
}

/************************************************************************/
/*                           DGNReadElement()                           */
/************************************************************************/

/**
 * Read a DGN element.
 *
 * This function will return the next element in the file, starting with the
 * first.  It is affected by DGNGotoElement() calls. 
 *
 * The element is read into a structure which includes the DGNElemCore 
 * structure.  It is expected that applications will inspect the stype
 * field of the returned DGNElemCore and use it to cast the pointer to the
 * appropriate element structure type such as DGNElemMultiPoint. 
 *
 * @param hDGN the handle of the file to read from.
 *
 * @return pointer to element structure, or NULL on EOF or processing error.
 * The structure should be freed with DGNFreeElement() when no longer needed.
 */

DGNElemCore *DGNReadElement( DGNHandle hDGN )

{
    DGNInfo	*psDGN = (DGNInfo *) hDGN;
    DGNElemCore *psElement = NULL;
    int		nType, nLevel;
    int         bInsideFilter;

/* -------------------------------------------------------------------- */
/*      Load the element data into the current buffer.  If a spatial    */
/*      filter is in effect, loop until we get something within our     */
/*      spatial constraints.                                            */
/* -------------------------------------------------------------------- */
    do { 
        bInsideFilter = TRUE;

        if( !DGNLoadRawElement( psDGN, &nType, &nLevel ) )
            return NULL;
        
        if( psDGN->has_spatial_filter )
        {
            GUInt32	nXMin, nXMax, nYMin, nYMax;
            
            if( !psDGN->sf_converted_to_uor )
                DGNSpatialFilterToUOR( psDGN );

            if( !DGNGetElementExtents( psDGN, nType, 
                                       &nXMin, &nYMin, NULL,
                                       &nXMax, &nYMax, NULL ) )
            {
                /* If we don't have spatial characterists for the element
                   we will pass it through. */
                bInsideFilter = TRUE;
            }
            else if( nXMin > psDGN->sf_max_x
                     || nYMin > psDGN->sf_max_y
                     || nXMax < psDGN->sf_min_x
                     || nYMax < psDGN->sf_min_y )
                bInsideFilter = FALSE;

            /*
            ** We want to select complex elements based on the extents of
            ** the header, not the individual elements.
            */
            if( psDGN->abyElem[0] & 0x80 /* complex flag set */ )
            {
                if( nType == DGNT_COMPLEX_CHAIN_HEADER
                    || nType == DGNT_COMPLEX_SHAPE_HEADER )
                {
                    psDGN->in_complex_group = TRUE;
                    psDGN->select_complex_group = bInsideFilter;
                }
                else if( psDGN->in_complex_group )
                    bInsideFilter = psDGN->select_complex_group;
            }
            else
                psDGN->in_complex_group = FALSE;
        }
    } while( !bInsideFilter );

/* -------------------------------------------------------------------- */
/*      Convert into an element structure.                              */
/* -------------------------------------------------------------------- */
    psElement = DGNProcessElement( psDGN, nType, nLevel );

    return psElement;
}

/************************************************************************/
/*                            DGNParseCore()                            */
/************************************************************************/

int DGNParseCore( DGNInfo *psDGN, DGNElemCore *psElement )

{
    GByte	*psData = psDGN->abyElem+0;

    psElement->level = psData[0] & 0x3f;
    psElement->complex = psData[0] & 0x80;
    psElement->deleted = psData[1] & 0x80;
    psElement->type = psData[1] & 0x7f;

    if( psDGN->nElemBytes >= 36 
        && psElement->type != DGNT_CELL_LIBRARY )
    {
        psElement->graphic_group = psData[28] + psData[29] * 256;
        psElement->properties = psData[32] + psData[33] * 256;
        psElement->style = psData[34] & 0x7;
        psElement->weight = (psData[34] & 0xf8) >> 3;
        psElement->color = psData[35];
    }

    if( psElement->properties & DGNPF_ATTRIBUTES )
    {
        int   nAttIndex;
        
        nAttIndex = psData[30] + psData[31] * 256;

        psElement->attr_bytes = psDGN->nElemBytes - nAttIndex*2 - 32;
        psElement->attr_data = (unsigned char *) 
            CPLMalloc(psElement->attr_bytes);
        memcpy( psElement->attr_data, psData + nAttIndex * 2 + 32,
                psElement->attr_bytes );
    }
    
    return TRUE;
}

/************************************************************************/
/*                         DGNParseColorTable()                         */
/************************************************************************/

static DGNElemCore *DGNParseColorTable( DGNInfo * psDGN )

{
    DGNElemCore *psElement;
    DGNElemColorTable  *psColorTable;
            
    psColorTable = (DGNElemColorTable *) 
        CPLCalloc(sizeof(DGNElemColorTable),1);
    psElement = (DGNElemCore *) psColorTable;
    psElement->stype = DGNST_COLORTABLE;

    DGNParseCore( psDGN, psElement );

    psColorTable->screen_flag = 
        psDGN->abyElem[36] + psDGN->abyElem[37] * 256;

    memcpy( psColorTable->color_info, psDGN->abyElem+41, 768 );	
    if( !psDGN->got_color_table )
    {
        memcpy( psDGN->color_table, psColorTable->color_info, 768 );
        psDGN->got_color_table = 1;
    }
    
    return psElement;
}

/************************************************************************/
/*                           DGNParseTagSet()                           */
/************************************************************************/

static DGNElemCore *DGNParseTagSet( DGNInfo * psDGN )

{
    DGNElemCore *psElement;
    DGNElemTagSet *psTagSet;
    int          nDataOffset, iTag;
            
    psTagSet = (DGNElemTagSet *) CPLCalloc(sizeof(DGNElemTagSet),1);
    psElement = (DGNElemCore *) psTagSet;
    psElement->stype = DGNST_TAG_SET;

    DGNParseCore( psDGN, psElement );

/* -------------------------------------------------------------------- */
/*      Parse the overall information.                                  */
/* -------------------------------------------------------------------- */
    psTagSet->tagCount = 
        psDGN->abyElem[44] + psDGN->abyElem[45] * 256;
    psTagSet->flags = 
        psDGN->abyElem[46] + psDGN->abyElem[47] * 256;
    psTagSet->tagSetName = CPLStrdup( (const char *) (psDGN->abyElem + 48) );

/* -------------------------------------------------------------------- */
/*      Get the tag set number out of the attributes, if available.     */
/* -------------------------------------------------------------------- */
    psTagSet->tagSet = -1;

    if( psElement->attr_bytes >= 8 
        && psElement->attr_data[0] == 0x03
        && psElement->attr_data[1] == 0x10
        && psElement->attr_data[2] == 0x2f
        && psElement->attr_data[3] == 0x7d )
        psTagSet->tagSet = psElement->attr_data[4]
            + psElement->attr_data[5] * 256;

/* -------------------------------------------------------------------- */
/*      Parse each of the tag definitions.                              */
/* -------------------------------------------------------------------- */
    psTagSet->tagList = (DGNTagDef *) 
        CPLMalloc(sizeof(DGNTagDef) * psTagSet->tagCount);

    nDataOffset = 48 + strlen(psTagSet->tagSetName) + 1 + 1; 

    for( iTag = 0; iTag < psTagSet->tagCount; iTag++ )
    {
        DGNTagDef *tagDef = psTagSet->tagList + iTag;

        CPLAssert( nDataOffset < psDGN->nElemBytes );

        /* collect tag name. */
        tagDef->name = CPLStrdup( (char *) psDGN->abyElem + nDataOffset );
        nDataOffset += strlen(tagDef->name)+1;

        /* Get tag id */
        tagDef->id = psDGN->abyElem[nDataOffset]
            + psDGN->abyElem[nDataOffset+1] * 256;
        nDataOffset += 2;

        /* Get User Prompt */
        tagDef->prompt = CPLStrdup( (char *) psDGN->abyElem + nDataOffset );
        nDataOffset += strlen(tagDef->prompt)+1;


        /* Get type */
        tagDef->type = psDGN->abyElem[nDataOffset]
            + psDGN->abyElem[nDataOffset+1] * 256;
        nDataOffset += 2;

        /* skip five zeros */
        nDataOffset += 5;

        /* Get the default */
        if( tagDef->type == 1 )
        {
            tagDef->defaultValue.string = 
                CPLStrdup( (char *) psDGN->abyElem + nDataOffset );
            nDataOffset += strlen(tagDef->defaultValue.string)+1;
        }
        else if( tagDef->type == 3 || tagDef->type == 5 )
        {
            memcpy( &(tagDef->defaultValue.integer), 
                    psDGN->abyElem + nDataOffset, 4 );
            tagDef->defaultValue.integer = 
                CPL_LSBWORD32( tagDef->defaultValue.integer );
            nDataOffset += 4;
        }
        else if( tagDef->type == 4 )
        {
            memcpy( &(tagDef->defaultValue.real), 
                    psDGN->abyElem + nDataOffset, 8 );
            DGN2IEEEDouble( &(tagDef->defaultValue.real) );
            nDataOffset += 8;
        }
        else
            nDataOffset += 4;
    }
    return psElement;
}

/************************************************************************/
/*                            DGNParseTCB()                             */
/************************************************************************/

static DGNElemCore *DGNParseTCB( DGNInfo * psDGN )

{
    DGNElemTCB *psTCB;
    DGNElemCore *psElement;

    psTCB = (DGNElemTCB *) CPLCalloc(sizeof(DGNElemTCB),1);
    psElement = (DGNElemCore *) psTCB;
    psElement->stype = DGNST_TCB;
    DGNParseCore( psDGN, psElement );

    if( psDGN->abyElem[1214] & 0x40 )
        psTCB->dimension = 3;
    else
        psTCB->dimension = 2;
          
    psTCB->subunits_per_master = DGN_INT32( psDGN->abyElem + 1112 );

    psTCB->master_units[0] = (char) psDGN->abyElem[1120];
    psTCB->master_units[1] = (char) psDGN->abyElem[1121];
    psTCB->master_units[2] = '\0';

    psTCB->uor_per_subunit = DGN_INT32( psDGN->abyElem + 1116 );

    psTCB->sub_units[0] = (char) psDGN->abyElem[1122];
    psTCB->sub_units[1] = (char) psDGN->abyElem[1123];
    psTCB->sub_units[2] = '\0';

    /* Get global origin */
    memcpy( &(psTCB->origin_x), psDGN->abyElem+1240, 8 );
    memcpy( &(psTCB->origin_y), psDGN->abyElem+1248, 8 );
    memcpy( &(psTCB->origin_z), psDGN->abyElem+1256, 8 );

    /* Transform to IEEE */
    DGN2IEEEDouble( &(psTCB->origin_x) );
    DGN2IEEEDouble( &(psTCB->origin_y) );
    DGN2IEEEDouble( &(psTCB->origin_z) );

    /* Convert from UORs to master units. */
    if( psTCB->uor_per_subunit != 0
        && psTCB->subunits_per_master != 0 )
    {
        psTCB->origin_x = psTCB->origin_x / 
            (psTCB->uor_per_subunit * psTCB->subunits_per_master);
        psTCB->origin_y = psTCB->origin_y / 
            (psTCB->uor_per_subunit * psTCB->subunits_per_master);
        psTCB->origin_z = psTCB->origin_z / 
            (psTCB->uor_per_subunit * psTCB->subunits_per_master);
    }

    if( !psDGN->got_tcb )
    {
        psDGN->got_tcb = TRUE;
        psDGN->dimension = psTCB->dimension;
        psDGN->origin_x = psTCB->origin_x;
        psDGN->origin_y = psTCB->origin_y;
        psDGN->origin_z = psTCB->origin_z;

        if( psTCB->uor_per_subunit != 0
            && psTCB->subunits_per_master != 0 )
            psDGN->scale = 1.0 
                / (psTCB->uor_per_subunit * psTCB->subunits_per_master);
    }

    return psElement;
}

/************************************************************************/
/*                           DGNFreeElement()                           */
/************************************************************************/

/**
 * Free an element structure.
 *
 * This function will deallocate all resources associated with any element
 * structure returned by DGNReadElement(). 
 *
 * @param hDGN handle to file from which the element was read.
 * @param psElement the element structure returned by DGNReadElement().
 */

void DGNFreeElement( DGNHandle hDGN, DGNElemCore *psElement )

{
    if( psElement->attr_data != NULL )
        VSIFree( psElement->attr_data );

    if( psElement->raw_data != NULL )
        VSIFree( psElement->raw_data );

    if( psElement->stype == DGNST_TAG_SET )
    {
        int		iTag;

        DGNElemTagSet *psTagSet = (DGNElemTagSet *) psElement;
        CPLFree( psTagSet->tagSetName );

        for( iTag = 0; iTag < psTagSet->tagCount; iTag++ )
        {
            CPLFree( psTagSet->tagList[iTag].name );
            CPLFree( psTagSet->tagList[iTag].prompt );

            if( psTagSet->tagList[iTag].type == 4 )
                CPLFree( psTagSet->tagList[iTag].defaultValue.string );
        }
    }
    else if( psElement->stype == DGNST_TAG_VALUE )
    {
        if( ((DGNElemTagValue *) psElement)->tagType == 4 )
            CPLFree( ((DGNElemTagValue *) psElement)->tagValue.string );
    }

    CPLFree( psElement );
}

/************************************************************************/
/*                             DGNRewind()                              */
/************************************************************************/

/**
 * Rewind element reading.
 *
 * Rewind the indicated DGN file, so the next element read with 
 * DGNReadElement() will be the first.  Does not require indexing like
 * the more general DGNReadElement() function.
 *
 * @param hDGN handle to file.
 */

void DGNRewind( DGNHandle hDGN )

{
    DGNInfo	*psDGN = (DGNInfo *) hDGN;

    VSIRewind( psDGN->fp );

    psDGN->next_element_id = 0;
    psDGN->in_complex_group = FALSE;
}

/************************************************************************/
/*                         DGNTransformPoint()                          */
/************************************************************************/

void DGNTransformPoint( DGNInfo *psDGN, DGNPoint *psPoint )

{
    psPoint->x = psPoint->x * psDGN->scale - psDGN->origin_x;
    psPoint->y = psPoint->y * psDGN->scale - psDGN->origin_y;
    psPoint->z = psPoint->z * psDGN->scale - psDGN->origin_z;
}

/************************************************************************/
/*                      DGNInverseTransformPoint()                      */
/************************************************************************/

void DGNInverseTransformPoint( DGNInfo *psDGN, DGNPoint *psPoint )

{
    psPoint->x = (psPoint->x + psDGN->origin_x) / psDGN->scale;
    psPoint->y = (psPoint->y + psDGN->origin_y) / psDGN->scale;
    psPoint->z = (psPoint->z + psDGN->origin_z) / psDGN->scale;

    psPoint->x = MAX(-2147483647,MIN(2147483647,psPoint->x));
    psPoint->y = MAX(-2147483647,MIN(2147483647,psPoint->y));
    psPoint->z = MAX(-2147483647,MIN(2147483647,psPoint->z));
}

/************************************************************************/
/*                         DGNGetElementIndex()                         */
/************************************************************************/

/**
 * Fetch element index.
 *
 * This function will return an array with brief information about every
 * element in a DGN file.  It requires one pass through the entire file to
 * generate (this is not repeated on subsequent calls). 
 *
 * The returned array of DGNElementInfo structures contain the level, type, 
 * stype, and other flags for each element in the file.  This can facilitate
 * application level code representing the number of elements of various types
 * effeciently. 
 *
 * Note that while building the index requires one pass through the whole file,
 * it does not generally request much processing for each element. 
 *
 * @param hDGN the file to get an index for.
 * @param pnElementCount the integer to put the total element count into. 
 *
 * @return a pointer to an internal array of DGNElementInfo structures (there 
 * will be *pnElementCount entries in the array), or NULL on failure.  The
 * returned array should not be modified or freed, and will last only as long
 * as the DGN file remains open. 
 */

const DGNElementInfo *DGNGetElementIndex( DGNHandle hDGN, int *pnElementCount )

{
    DGNInfo	*psDGN = (DGNInfo *) hDGN;

    DGNBuildIndex( psDGN );

    if( pnElementCount != NULL )
        *pnElementCount = psDGN->element_count;
    
    return psDGN->element_index;
}

/************************************************************************/
/*                           DGNGetExtents()                            */
/************************************************************************/

/**
 * Fetch overall file extents.
 *
 * The extents are collected for each element while building an index, so
 * if an index has not already been built, it will be built when 
 * DGNGetExtents() is called.  
 * 
 * The Z min/max values are generally meaningless (0 and 0xffffffff in uor
 * space). 
 * 
 * @param hDGN the file to get extents for.
 * @param padfExtents pointer to an array of six doubles into which are loaded
 * the values xmin, ymin, zmin, xmax, ymax, and zmax.
 *
 * @return TRUE on success or FALSE on failure.
 */

int DGNGetExtents( DGNHandle hDGN, double * padfExtents )

{
    DGNInfo	*psDGN = (DGNInfo *) hDGN;
    DGNPoint	sMin, sMax;

    DGNBuildIndex( psDGN );

    if( !psDGN->got_bounds )
        return FALSE;

    sMin.x = psDGN->min_x - 2147483648.0;
    sMin.y = psDGN->min_y - 2147483648.0;
    sMin.z = psDGN->min_z - 2147483648.0;
    
    DGNTransformPoint( psDGN, &sMin );

    padfExtents[0] = sMin.x;
    padfExtents[1] = sMin.y;
    padfExtents[2] = sMin.z;
    
    sMax.x = psDGN->max_x - 2147483648.0;
    sMax.y = psDGN->max_y - 2147483648.0;
    sMax.z = psDGN->max_z - 2147483648.0;

    DGNTransformPoint( psDGN, &sMax );

    padfExtents[3] = sMax.x;
    padfExtents[4] = sMax.y;
    padfExtents[5] = sMax.z;

    return TRUE;
}

/************************************************************************/
/*                           DGNBuildIndex()                            */
/************************************************************************/

void DGNBuildIndex( DGNInfo *psDGN )

{
    int	nMaxElements, nType, nLevel;
    long nLastOffset;
    GUInt32 anRegion[6];

    if( psDGN->index_built ) 
        return;

    psDGN->index_built = TRUE;
    
    DGNRewind( psDGN );

    nMaxElements = 0;

    nLastOffset = VSIFTell( psDGN->fp );
    while( DGNLoadRawElement( psDGN, &nType, &nLevel ) )
    {
        DGNElementInfo	*psEI;

        if( psDGN->element_count == nMaxElements )
        {
            nMaxElements = (int) (nMaxElements * 1.5) + 500;
            
            psDGN->element_index = (DGNElementInfo *) 
                CPLRealloc( psDGN->element_index, 
                            nMaxElements * sizeof(DGNElementInfo) );
        }

        psEI = psDGN->element_index + psDGN->element_count;
        psEI->level = nLevel;
        psEI->type = nType;
        psEI->flags = 0;
        psEI->offset = nLastOffset;

        if( psDGN->abyElem[0] & 0x80 )
            psEI->flags |= DGNEIF_COMPLEX;

        if( psDGN->abyElem[1] & 0x80 )
            psEI->flags |= DGNEIF_DELETED;

        if( nType == DGNT_LINE || nType == DGNT_LINE_STRING
            || nType == DGNT_SHAPE || nType == DGNT_CURVE
            || nType == DGNT_BSPLINE )
            psEI->stype = DGNST_MULTIPOINT;

        else if( nType == DGNT_GROUP_DATA && nLevel == DGN_GDL_COLOR_TABLE )
        {
            DGNElemCore	*psCT = DGNParseColorTable( psDGN );
            DGNFreeElement( (DGNHandle) psDGN, psCT );
            psEI->stype = DGNST_COLORTABLE;
        }
        else if( nType == DGNT_ELLIPSE || nType == DGNT_ARC )
            psEI->stype = DGNST_ARC;
        
        else if( nType == DGNT_COMPLEX_SHAPE_HEADER 
                 || nType == DGNT_COMPLEX_CHAIN_HEADER )
            psEI->stype = DGNST_COMPLEX_HEADER;
        
        else if( nType == DGNT_TEXT )
            psEI->stype = DGNST_TEXT;

        else if( nType == DGNT_TAG_VALUE )
            psEI->stype = DGNST_TAG_VALUE;

        else if( nType == DGNT_APPLICATION_ELEM )
        {
            if( nLevel == 24 )
                psEI->stype = DGNST_TAG_SET;
            else
                psEI->stype = DGNST_CORE;
        }
        else if( nType == DGNT_TCB )
        {
            DGNElemCore	*psTCB = DGNParseTCB( psDGN );
            DGNFreeElement( (DGNHandle) psDGN, psTCB );
            psEI->stype = DGNST_TCB;
        }
        else
            psEI->stype = DGNST_CORE;

        if( !(psEI->flags & DGNEIF_DELETED)
            && !(psEI->flags & DGNEIF_COMPLEX) 
            && DGNGetElementExtents( psDGN, nType, 
                                     anRegion+0, anRegion+1, anRegion+2,
                                     anRegion+3, anRegion+4, anRegion+5 ) )
        {
#ifdef notdef
            printf( "panRegion[%d]=%.1f,%.1f,%.1f,%.1f,%.1f,%.1f\n", 
                    psDGN->element_count,
                    anRegion[0] - 2147483648.0,
                    anRegion[1] - 2147483648.0,
                    anRegion[2] - 2147483648.0,
                    anRegion[3] - 2147483648.0,
                    anRegion[4] - 2147483648.0,
                    anRegion[5] - 2147483648.0 );
#endif            
            if( psDGN->got_bounds )
            {
                psDGN->min_x = MIN(psDGN->min_x, anRegion[0]);
                psDGN->min_y = MIN(psDGN->min_y, anRegion[1]);
                psDGN->min_z = MIN(psDGN->min_z, anRegion[2]);
                psDGN->max_x = MAX(psDGN->max_x, anRegion[3]);
                psDGN->max_y = MAX(psDGN->max_y, anRegion[4]);
                psDGN->max_z = MAX(psDGN->max_z, anRegion[5]);
            }
            else
            {
                memcpy( &(psDGN->min_x), anRegion, sizeof(GInt32) * 6 );
                psDGN->got_bounds = TRUE;
            }
        }

        psDGN->element_count++;

        nLastOffset = VSIFTell( psDGN->fp );
    }

    DGNRewind( psDGN );

    psDGN->max_element_count = nMaxElements;
}

