/*
 * Copyright (C) 2008 ZXing authors
 * Copyright 2011 Robert Theis
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package edu.sfsu.cs.orange.ocr;

import java.util.List;

import edu.sfsu.cs.orange.ocr.R;
import edu.sfsu.cs.orange.ocr.camera.CameraManager;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Align;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.Paint.Style;
import android.util.AttributeSet;
import android.view.View;

/**
 * This view is overlaid on top of the camera preview. It adds the viewfinder rectangle and partial
 * transparency outside it, as well as the result text.
 *
 * The code for this class was adapted from the ZXing project: http://code.google.com/p/zxing
 */
public final class ViewfinderView extends View {
  //private static final long ANIMATION_DELAY = 80L;

  /** Flag to draw boxes representing the results from TessBaseAPI::GetRegions(). */
  static final boolean DRAW_REGION_BOXES = false;

  /** Flag to draw boxes representing the results from TessBaseAPI::GetTextlines(). */
  static final boolean DRAW_TEXTLINE_BOXES = true;

  /** Flag to draw boxes representing the results from TessBaseAPI::GetStrips(). */
  static final boolean DRAW_STRIP_BOXES = false;

  /** Flag to draw boxes representing the results from TessBaseAPI::GetWords(). */
  static final boolean DRAW_WORD_BOXES = true;

  /** Flag to draw word text with a background varying from transparent to opaque. */
  static final boolean DRAW_TRANSPARENT_WORD_BACKGROUNDS = false;

  /** Flag to draw boxes representing the results from TessBaseAPI::GetCharacters(). */
  static final boolean DRAW_CHARACTER_BOXES = false;

  /** Flag to draw the text of words within their respective boxes from TessBaseAPI::GetWords(). */
  static final boolean DRAW_WORD_TEXT = false;

  /** Flag to draw each character in its respective box from TessBaseAPI::GetCharacters(). */
  static final boolean DRAW_CHARACTER_TEXT = false;

  private CameraManager cameraManager;
  private final Paint paint;
  private final int maskColor;
  private final int frameColor;
  private final int cornerColor;
  private OcrResultText resultText;
  private String[] words;
  private List<Rect> regionBoundingBoxes;
  private List<Rect> textlineBoundingBoxes;
  private List<Rect> stripBoundingBoxes;
  private List<Rect> wordBoundingBoxes;
  private List<Rect> characterBoundingBoxes;
  //  Rect bounds;
  private Rect previewFrame;
  private Rect rect;

  // This constructor is used when the class is built from an XML resource.
  public ViewfinderView(Context context, AttributeSet attrs) {
    super(context, attrs);

    // Initialize these once for performance rather than calling them every time in onDraw().
    paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    Resources resources = getResources();
    maskColor = resources.getColor(R.color.viewfinder_mask);
    frameColor = resources.getColor(R.color.viewfinder_frame);
    cornerColor = resources.getColor(R.color.viewfinder_corners);

    //    bounds = new Rect();
    previewFrame = new Rect();
    rect = new Rect();
  }

  public void setCameraManager(CameraManager cameraManager) {
    this.cameraManager = cameraManager;
  }

  @SuppressWarnings("unused")
  @Override
  public void onDraw(Canvas canvas) {
    Rect frame = cameraManager.getFramingRect();
    if (frame == null) {
      return;
    }
    int width = canvas.getWidth();
    int height = canvas.getHeight(); 

    // Draw the exterior (i.e. outside the framing rect) darkened
    paint.setColor(maskColor);
    canvas.drawRect(0, 0, width, frame.top, paint);
    canvas.drawRect(0, frame.top, frame.left, frame.bottom + 1, paint);
    canvas.drawRect(frame.right + 1, frame.top, width, frame.bottom + 1, paint);
    canvas.drawRect(0, frame.bottom + 1, width, height, paint);

    // If we have an OCR result, overlay its information on the viewfinder.
    if (resultText != null) {

      // Only draw text/bounding boxes on viewfinder if it hasn't been resized since the OCR was requested.
      Point bitmapSize = resultText.getBitmapDimensions();
      previewFrame = cameraManager.getFramingRectInPreview();
      if (bitmapSize.x == previewFrame.width() && bitmapSize.y == previewFrame.height()) {


        float scaleX = frame.width() / (float) previewFrame.width();
        float scaleY = frame.height() / (float) previewFrame.height();

        if (DRAW_REGION_BOXES) {
          regionBoundingBoxes = resultText.getRegionBoundingBoxes();
          for (int i = 0; i < regionBoundingBoxes.size(); i++) {
            paint.setAlpha(0xA0);
            paint.setColor(Color.MAGENTA);
            paint.setStyle(Style.STROKE);
            paint.setStrokeWidth(1);
            rect = regionBoundingBoxes.get(i);
            canvas.drawRect(frame.left + rect.left * scaleX,
                frame.top + rect.top * scaleY, 
                frame.left + rect.right * scaleX, 
                frame.top + rect.bottom * scaleY, paint);
          }      
        }

        if (DRAW_TEXTLINE_BOXES) {
          // Draw each textline
          textlineBoundingBoxes = resultText.getTextlineBoundingBoxes();
          paint.setAlpha(0xA0);
          paint.setColor(Color.RED);
          paint.setStyle(Style.STROKE);
          paint.setStrokeWidth(1);
          for (int i = 0; i < textlineBoundingBoxes.size(); i++) {
            rect = textlineBoundingBoxes.get(i);
            canvas.drawRect(frame.left + rect.left * scaleX,
                frame.top + rect.top * scaleY, 
                frame.left + rect.right * scaleX, 
                frame.top + rect.bottom * scaleY, paint);
          }
        }

        if (DRAW_STRIP_BOXES) {
          stripBoundingBoxes = resultText.getStripBoundingBoxes();
          paint.setAlpha(0xFF);
          paint.setColor(Color.YELLOW);
          paint.setStyle(Style.STROKE);
          paint.setStrokeWidth(1);
          for (int i = 0; i < stripBoundingBoxes.size(); i++) {
            rect = stripBoundingBoxes.get(i);
            canvas.drawRect(frame.left + rect.left * scaleX,
                frame.top + rect.top * scaleY, 
                frame.left + rect.right * scaleX, 
                frame.top + rect.bottom * scaleY, paint);
          }        	
        }

        if (DRAW_WORD_BOXES || DRAW_WORD_TEXT) {
          // Split the text into words
          wordBoundingBoxes = resultText.getWordBoundingBoxes();
          //      for (String w : words) {
          //        Log.e("ViewfinderView", "word: " + w);
          //      }
          //Log.d("ViewfinderView", "There are " + words.length + " words in the string array.");
          //Log.d("ViewfinderView", "There are " + wordBoundingBoxes.size() + " words with bounding boxes.");
        }

        if (DRAW_WORD_BOXES) {
          paint.setAlpha(0xFF);
          paint.setColor(0xFF00CCFF);
          paint.setStyle(Style.STROKE);
          paint.setStrokeWidth(1);
          for (int i = 0; i < wordBoundingBoxes.size(); i++) {
            // Draw a bounding box around the word
            rect = wordBoundingBoxes.get(i);
            canvas.drawRect(
                frame.left + rect.left * scaleX,
                frame.top + rect.top * scaleY, 
                frame.left + rect.right * scaleX, 
                frame.top + rect.bottom * scaleY, paint);
          }
        }  

        if (DRAW_WORD_TEXT) { 
          words = resultText.getText().replace("\n"," ").split(" ");
          int[] wordConfidences = resultText.getWordConfidences();          
          for (int i = 0; i < wordBoundingBoxes.size(); i++) {
            boolean isWordBlank = true;
            try {
              if (!words[i].equals("")) {
                isWordBlank = false;
              }
            } catch (ArrayIndexOutOfBoundsException e) {
              e.printStackTrace();
            }

            // Only draw if word has characters
            if (!isWordBlank) {
              // Draw a white background around each word
              rect = wordBoundingBoxes.get(i);
              paint.setColor(Color.WHITE);
              paint.setStyle(Style.FILL);
              if (DRAW_TRANSPARENT_WORD_BACKGROUNDS) {
                // Higher confidence = more opaque, less transparent background
                paint.setAlpha(wordConfidences[i] * 255 / 100);
              } else {
                paint.setAlpha(255);
              }
              canvas.drawRect(frame.left + rect.left * scaleX,
                  frame.top + rect.top * scaleY, 
                  frame.left + rect.right * scaleX, 
                  frame.top + rect.bottom * scaleY, paint);

              // Draw the word in black text
              paint.setColor(Color.BLACK);
              paint.setAlpha(0xFF);
              paint.setAntiAlias(true);
              paint.setTextAlign(Align.LEFT);

              // Adjust text size to fill rect
              paint.setTextSize(100);
              paint.setTextScaleX(1.0f);
              // ask the paint for the bounding rect if it were to draw this text
              Rect bounds = new Rect();
              paint.getTextBounds(words[i], 0, words[i].length(), bounds);
              // get the height that would have been produced
              int h = bounds.bottom - bounds.top;
              // figure out what textSize setting would create that height of text
              float size  = (((float)(rect.height())/h)*100f);
              // and set it into the paint
              paint.setTextSize(size);
              // Now set the scale.
              // do calculation with scale of 1.0 (no scale)
              paint.setTextScaleX(1.0f);
              // ask the paint for the bounding rect if it were to draw this text.
              paint.getTextBounds(words[i], 0, words[i].length(), bounds);
              // determine the width
              int w = bounds.right - bounds.left;
              // calculate the baseline to use so that the entire text is visible including the descenders
              int text_h = bounds.bottom-bounds.top;
              int baseline =bounds.bottom+((rect.height()-text_h)/2);
              // determine how much to scale the width to fit the view
              float xscale = ((float) (rect.width())) / w;
              // set the scale for the text paint
              paint.setTextScaleX(xscale);
              canvas.drawText(words[i], frame.left + rect.left * scaleX, frame.top + rect.bottom * scaleY - baseline, paint);
            }

          }
        }  

//        if (DRAW_CHARACTER_BOXES || DRAW_CHARACTER_TEXT) {
//          characterBoundingBoxes = resultText.getCharacterBoundingBoxes();
//        }
//
//        if (DRAW_CHARACTER_BOXES) {
//          // Draw bounding boxes around each character
//          paint.setAlpha(0xA0);
//          paint.setColor(0xFF00FF00);
//          paint.setStyle(Style.STROKE);
//          paint.setStrokeWidth(1);
//          for (int c = 0; c < characterBoundingBoxes.size(); c++) {
//            Rect characterRect = characterBoundingBoxes.get(c);
//            canvas.drawRect(frame.left + characterRect.left * scaleX,
//                frame.top + characterRect.top * scaleY, 
//                frame.left + characterRect.right * scaleX, 
//                frame.top + characterRect.bottom * scaleY, paint);
//          }
//        }
//
//        if (DRAW_CHARACTER_TEXT) {
//          // Draw letters individually
//          for (int i = 0; i < characterBoundingBoxes.size(); i++) {
//            Rect r = characterBoundingBoxes.get(i);
//
//            // Draw a white background for every letter
//            int meanConfidence = resultText.getMeanConfidence();
//            paint.setColor(Color.WHITE);
//            paint.setAlpha(meanConfidence * (255 / 100));
//            paint.setStyle(Style.FILL);
//            canvas.drawRect(frame.left + r.left * scaleX,
//                frame.top + r.top * scaleY, 
//                frame.left + r.right * scaleX, 
//                frame.top + r.bottom * scaleY, paint);
//
//            // Draw each letter, in black
//            paint.setColor(Color.BLACK);
//            paint.setAlpha(0xFF);
//            paint.setAntiAlias(true);
//            paint.setTextAlign(Align.LEFT);
//            String letter = "";
//            try {
//              char c = resultText.getText().replace("\n","").replace(" ", "").charAt(i);
//              letter = Character.toString(c);
//
//              if (!letter.equals("-") && !letter.equals("_")) {
//
//                // Adjust text size to fill rect
//                paint.setTextSize(100);
//                paint.setTextScaleX(1.0f);
//
//                // ask the paint for the bounding rect if it were to draw this text
//                Rect bounds = new Rect();
//                paint.getTextBounds(letter, 0, letter.length(), bounds);
//
//                // get the height that would have been produced
//                int h = bounds.bottom - bounds.top;
//
//                // figure out what textSize setting would create that height of text
//                float size  = (((float)(r.height())/h)*100f);
//
//                // and set it into the paint
//                paint.setTextSize(size);
//
//                // Draw the text as is. We don't really need to set the text scale, because the dimensions
//                // of the Rect should already be suited for drawing our letter. 
//                canvas.drawText(letter, frame.left + r.left * scaleX, frame.top + r.bottom * scaleY, paint);
//              }
//            } catch (StringIndexOutOfBoundsException e) {
//              e.printStackTrace();
//            } catch (Exception e) {
//              e.printStackTrace();
//            }
//          }
//        }
      }

    }
    // Draw a two pixel solid border inside the framing rect
    paint.setAlpha(0);
    paint.setStyle(Style.FILL);
    paint.setColor(frameColor);
    canvas.drawRect(frame.left, frame.top, frame.right + 1, frame.top + 2, paint);
    canvas.drawRect(frame.left, frame.top + 2, frame.left + 2, frame.bottom - 1, paint);
    canvas.drawRect(frame.right - 1, frame.top, frame.right + 1, frame.bottom - 1, paint);
    canvas.drawRect(frame.left, frame.bottom - 1, frame.right + 1, frame.bottom + 1, paint);

    // Draw the framing rect corner UI elements
    paint.setColor(cornerColor);
    canvas.drawRect(frame.left - 15, frame.top - 15, frame.left + 15, frame.top, paint);
    canvas.drawRect(frame.left - 15, frame.top, frame.left, frame.top + 15, paint);
    canvas.drawRect(frame.right - 15, frame.top - 15, frame.right + 15, frame.top, paint);
    canvas.drawRect(frame.right, frame.top - 15, frame.right + 15, frame.top + 15, paint);
    canvas.drawRect(frame.left - 15, frame.bottom, frame.left + 15, frame.bottom + 15, paint);
    canvas.drawRect(frame.left - 15, frame.bottom - 15, frame.left, frame.bottom, paint);
    canvas.drawRect(frame.right - 15, frame.bottom, frame.right + 15, frame.bottom + 15, paint);
    canvas.drawRect(frame.right, frame.bottom - 15, frame.right + 15, frame.bottom + 15, paint);  


    // Request another update at the animation interval, but don't repaint the entire viewfinder mask.
    //postInvalidateDelayed(ANIMATION_DELAY, frame.left, frame.top, frame.right, frame.bottom);
  }

  public void drawViewfinder() {
    invalidate();
  }

  /**
   * Adds the given OCR results for drawing to the view.
   * 
   * @param text Object containing OCR-derived text and corresponding data.
   */
  public void addResultText(OcrResultText text) {
    resultText = text; 
  }

  /**
   * Nullifies OCR text to remove it at the next onDraw() drawing.
   */
  public void removeResultText() {
    resultText = null;
  }
}
