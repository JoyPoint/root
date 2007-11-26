// @(#)root/eve:$Id$
// Authors: Matevz Tadel & Alja Mrak-Tadel: 2006, 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include <TEveTriangleSetEditor.h>
#include <TEveTriangleSet.h>
#include <TEveTransEditor.h>

#include <TVirtualPad.h>
#include <TColor.h>

#include <TGLabel.h>
#include <TGButton.h>
#include <TGNumberEntry.h>
#include <TGColorSelect.h>
#include <TGDoubleSlider.h>

//______________________________________________________________________________
// TEveTriangleSetEditor
//
// Editor for TEveTriangleSet class.

ClassImp(TEveTriangleSetEditor)

//______________________________________________________________________________
TEveTriangleSetEditor::TEveTriangleSetEditor(const TGWindow *p, Int_t width, Int_t height,
                                             UInt_t options, Pixel_t back) :
   TGedFrame(p, width, height, options | kVerticalFrame, back),
   fM(0),
   fHMTrans(0)
{
   MakeTitle("TEveTriangleSet");

   fHMTrans = new TEveTransSubEditor(this);
   fHMTrans->Connect("UseTrans()",     "TEveTriangleSetEditor", this, "Update()");
   fHMTrans->Connect("TransChanged()", "TEveTriangleSetEditor", this, "Update()");
   AddFrame(fHMTrans, new TGLayoutHints(kLHintsTop | kLHintsExpandX, 2, 0, 0, 0));
}

//______________________________________________________________________________
TEveTriangleSetEditor::~TEveTriangleSetEditor()
{
   delete fHMTrans;
}

/******************************************************************************/

//______________________________________________________________________________
void TEveTriangleSetEditor::SetModel(TObject* obj)
{
   fM = dynamic_cast<TEveTriangleSet*>(obj);

   fHMTrans->SetDataFromTrans(&fM->fHMTrans);
}
