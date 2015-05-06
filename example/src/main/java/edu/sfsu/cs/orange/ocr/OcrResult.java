/*
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

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.Point;
import android.graphics.Rect;

/**
 * Encapsulates the result of OCR.
 */
public class OcrResult {
  private Bitmap bitmap;
  private String text;
  
  private int[] wordConfidences;
  private int meanConfidence;

  private List<Rect> regionBoundingBoxes;
  private List<Rect> textlineBoundingBoxes;
  private List<Rect> wordBoundingBoxes;
  private List<Rect> stripBoundingBoxes;  
  private List<Rect> characterBoundingBoxes;

  private long timestamp;
  private long recognitionTimeRequired;

  private Paint paint;
  
  public OcrResult(Bitmap bitmap,
                   String text,
                   int[] wordConfidences,
                   int meanConfidence,
                   List<Rect> regionBoundingBoxes,
                   List<Rect> textlineBoundingBoxes,
                   List<Rect> wordBoundingBoxes,
                   List<Rect> stripBoundingBoxes,
                   List<Rect> characterBoundingBoxes,
                   long recognitionTimeRequired) {
    this.bitmap = bitmap;
    this.text = text;
    this.wordConfidences = wordConfidences;
    this.meanConfidence = meanConfidence;
    this.regionBoundingBoxes = regionBoundingBoxes;
    this.textlineBoundingBoxes = textlineBoundingBoxes;
    this.wordBoundingBoxes = wordBoundingBoxes;
    this.stripBoundingBoxes = stripBoundingBoxes;
    this.characterBoundingBoxes = characterBoundingBoxes;
    this.recognitionTimeRequired = recognitionTimeRequired;
    this.timestamp = System.currentTimeMillis();
    
    this.paint = new Paint();
  }

  public OcrResult() {
    timestamp = System.currentTimeMillis();
    this.paint = new Paint();
  }

  public Bitmap getBitmap() {
    return getAnnotatedBitmap();
  }
  
  private Bitmap getAnnotatedBitmap() {
    Canvas canvas = new Canvas(bitmap);
    
    // Draw bounding boxes around each word
    for (int i = 0; i < wordBoundingBoxes.size(); i++) {
      paint.setAlpha(0xFF);
      paint.setColor(0xFF00CCFF);
      paint.setStyle(Style.STROKE);
      paint.setStrokeWidth(2);
      Rect r = wordBoundingBoxes.get(i);
      canvas.drawRect(r, paint);
    }    
    
//    // Draw bounding boxes around each character
//    for (int i = 0; i < characterBoundingBoxes.size(); i++) {
//      paint.setAlpha(0xA0);
//      paint.setColor(0xFF00FF00);
//      paint.setStyle(Style.STROKE);
//      paint.setStrokeWidth(3);
//      Rect r = characterBoundingBoxes.get(i);
//      canvas.drawRect(r, paint);
//    }
    
    return bitmap;
  }
  
  public String getText() {
    return text;
  }

  public int[] getWordConfidences() {
    return wordConfidences;
  }

  public int getMeanConfidence() {
    return meanConfidence;
  }

  public long getRecognitionTimeRequired() {
    return recognitionTimeRequired;
  }

  public Point getBitmapDimensions() {
    return new Point(bitmap.getWidth(), bitmap.getHeight()); 
  }
  
  public List<Rect> getRegionBoundingBoxes() {
    return regionBoundingBoxes;
  }
  
  public List<Rect> getTextlineBoundingBoxes() {
    return textlineBoundingBoxes;
  }
  
  public List<Rect> getWordBoundingBoxes() {
    return wordBoundingBoxes;
  }
  
  public List<Rect> getStripBoundingBoxes() {
  	return stripBoundingBoxes;
  }
  
  public List<Rect> getCharacterBoundingBoxes() {
    return characterBoundingBoxes;
  }
  
  public long getTimestamp() {
    return timestamp;
  }
  
  public void setBitmap(Bitmap bitmap) {
    this.bitmap = bitmap;
  }
  
  public void setText(String text) {
    this.text = text;
  }

  public void setWordConfidences(int[] wordConfidences) {
    this.wordConfidences = wordConfidences;
  }

  public void setMeanConfidence(int meanConfidence) {
    this.meanConfidence = meanConfidence;
  }

  public void setRecognitionTimeRequired(long recognitionTimeRequired) {
    this.recognitionTimeRequired = recognitionTimeRequired;
  }
  
  public void setRegionBoundingBoxes(List<Rect> regionBoundingBoxes) {
    this.regionBoundingBoxes = regionBoundingBoxes;
  }
  
  public void setTextlineBoundingBoxes(List<Rect> textlineBoundingBoxes) {
    this.textlineBoundingBoxes = textlineBoundingBoxes;
  }

  public void setWordBoundingBoxes(List<Rect> wordBoundingBoxes) {
    this.wordBoundingBoxes = wordBoundingBoxes;
  }
  
  public void setStripBoundingBoxes(List<Rect> stripBoundingBoxes) {
  	this.stripBoundingBoxes = stripBoundingBoxes;
  }

  public void setCharacterBoundingBoxes(List<Rect> characterBoundingBoxes) {
    this.characterBoundingBoxes = characterBoundingBoxes;
  }
  
  @Override
  public String toString() {
    return text + " " + meanConfidence + " " + recognitionTimeRequired + " " + timestamp;
  }
}
