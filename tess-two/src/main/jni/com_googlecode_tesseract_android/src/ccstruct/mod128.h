/**********************************************************************
 * File:        mod128.h  (Formerly dir128.h)
 * Description: Header for class which implements modulo arithmetic.
 * Author:					Ray Smith
 * Created:					Tue Mar 26 17:48:13 GMT 1991
 *
 * (C) Copyright 1991, Hewlett-Packard Ltd.
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 ** http://www.apache.org/licenses/LICENSE-2.0
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 *
 **********************************************************************/

#ifndef           MOD128_H
#define           MOD128_H

#include          "points.h"

#define MODULUS       128        /*range of directions */
#define DIRBITS       7          //no of bits used
#define DIRSCALE      1000       //length of vector

class DLLSYM DIR128
{
  public:
    DIR128() {
    }                            //empty constructor

    DIR128(                //constructor
           inT16 value) {  //value to assign
      value %= MODULUS;          //modulo arithmetic
      if (value < 0)
        value += MODULUS;        //done properly
      dir = (inT8) value;
    }
    DIR128(const FCOORD fc);  //quantize vector

    DIR128 & operator= (         //assign of inT16
    inT16 value) {               //value to assign
      value %= MODULUS;          //modulo arithmetic
      if (value < 0)
        value += MODULUS;        //done properly
      dir = (inT8) value;
      return *this;
    }
    inT8 operator- (             //subtraction
      const DIR128 & minus) const//for signed result
    {
                                 //result
      inT16 result = dir - minus.dir;

      if (result > MODULUS / 2)
        result -= MODULUS;       //get in range
      else if (result < -MODULUS / 2)
        result += MODULUS;
      return (inT8) result;
    }
    DIR128 operator+ (           //addition
      const DIR128 & add) const  //of itself
    {
      DIR128 result;             //sum

      result = dir + add.dir;    //let = do the work
      return result;
    }
    DIR128 & operator+= (        //same as +
    const DIR128 & add) {
      *this = dir + add.dir;     //let = do the work
      return *this;
    }
    inT8 get_dir() const {  //access function
      return dir;
    }
    ICOORD vector() const;  //turn to vector

  private:
    inT8 dir;                    //a direction
};
#endif
