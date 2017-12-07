#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Set some language specific variables. Works in conjunction with
# tesstrain.sh
#

#=============================================================================
# Language specific info
#=============================================================================

# Array of all valid language codes.
VALID_LANGUAGE_CODES="afr amh ara asm aze aze_cyrl bel ben bih bod bos bul cat
                      ceb ces chi_sim chi_tra chr cym cyr_lid dan deu div dzo
                      ell eng enm epo est eus fas fil fin fra frk frm gle glg
                      grc guj hat heb hin hrv hun hye iku ind isl ita ita_old
                      jav jpn kan kat kat_old kaz khm kir kor kur lao lat
                      lat_lid lav lit mal mar mkd mlt msa mya nep nld nor ori
                      pan pol por pus ron rus san sin slk slv snd spa spa_old
                      sqi srp srp_latn swa swe syr tam tel tgk tgl tha tir tur
                      uig ukr urd uzb uzb_cyrl vie yid"

# Codes for which we have webtext but no fonts:
# armenian, dhivehi, mongolian (we support mongolian cyrillic as in the webtext,
# but not mongolian script with vertical writing direction), sindhi (for which
# we have persian script webtext, but real sindhi text can be in persian OR
# devanagari script)
UNUSABLE_LANGUAGE_CODES="hye div mon snd"

FRAKTUR_FONTS=(
    "CaslonishFraxx Medium" \
    "Cloister Black, Light" \
    "Proclamate Light" \
    "UnifrakturMaguntia" \
    "Walbaum-Fraktur" \
)

# List of fonts to train on
LATIN_FONTS=(
    "Arial Bold" \
    "Arial Bold Italic" \
    "Arial Italic" \
    "Arial" \
    "Courier New Bold" \
    "Courier New Bold Italic" \
    "Courier New Italic" \
    "Courier New" \
    "Times New Roman, Bold" \
    "Times New Roman, Bold Italic" \
    "Times New Roman, Italic" \
    "Times New Roman," \
    "Georgia Bold" \
    "Georgia Italic" \
    "Georgia" \
    "Georgia Bold Italic" \
    "Trebuchet MS Bold" \
    "Trebuchet MS Bold Italic" \
    "Trebuchet MS Italic" \
    "Trebuchet MS" \
    "Verdana Bold" \
    "Verdana Italic" \
    "Verdana" \
    "Verdana Bold Italic" \
    "URW Bookman L Bold" \
    "URW Bookman L Italic" \
    "URW Bookman L Bold Italic" \
    "Century Schoolbook L Bold" \
    "Century Schoolbook L Italic" \
    "Century Schoolbook L Bold Italic" \
    "Century Schoolbook L Medium" \
    "DejaVu Sans Ultra-Light" \
)

# List of fonts for printed/neo-Latin ('lat' language code, different from Latin script)
NEOLATIN_FONTS=(
    "GFS Bodoni" \
    "GFS Bodoni Bold" \
    "GFS Bodoni Italic" \
    "GFS Bodoni Bold Italic" \
    "GFS Didot" \
    "GFS Didot Bold" \
    "GFS Didot Italic" \
    "GFS Didot Bold Italic" \
    "Cardo" \
    "Cardo Bold" \
    "Cardo Italic" \
    "Wyld" \
    "Wyld Italic" \
    "EB Garamond" \
    "EB Garamond Italic" \
    "Junicode" \
    "Junicode Bold" \
    "Junicode Italic" \
    "Junicode Bold Italic" \
    "IM FELL DW Pica PRO" \
    "IM FELL English PRO" \
    "IM FELL Double Pica PRO" \
    "IM FELL French Canon PRO" \
    "IM FELL Great Primer PRO" \
    "IM FELL DW Pica PRO Italic" \
    "IM FELL English PRO Italic" \
    "IM FELL Double Pica PRO Italic" \
    "IM FELL French Canon PRO Italic" \
    "IM FELL Great Primer PRO Italic" \
)

EARLY_LATIN_FONTS=(
    "${FRAKTUR_FONTS[@]}" \
    "${LATIN_FONTS[@]}" \
    # The Wyld font family renders early modern ligatures encoded in the private
    # unicode area.
    "Wyld" \
    "Wyld Italic" \
    # Fonts that render the Yogh symbol (U+021C, U+021D) found in Old English.
    "GentiumAlt" \
)

VIETNAMESE_FONTS=( \
    "Arial Unicode MS Bold" \
    "Arial Bold Italic" \
    "Arial Italic" \
    "Arial Unicode MS" \
    "FreeMono Bold" \
    "Courier New Bold Italic" \
    "FreeMono Italic" \
    "FreeMono" \
    "GentiumAlt Italic" \
    "GentiumAlt" \
    "Palatino Linotype Bold" \
    "Palatino Linotype Bold Italic" \
    "Palatino Linotype Italic" \
    "Palatino Linotype" \
    "Really No 2 LT W2G Light" \
    "Really No 2 LT W2G Light Italic" \
    "Really No 2 LT W2G Medium" \
    "Really No 2 LT W2G Medium Italic" \
    "Really No 2 LT W2G Semi-Bold" \
    "Really No 2 LT W2G Semi-Bold Italic" \
    "Really No 2 LT W2G Ultra-Bold" \
    "Really No 2 LT W2G Ultra-Bold Italic" \
    "Times New Roman, Bold" \
    "Times New Roman, Bold Italic" \
    "Times New Roman, Italic" \
    "Times New Roman," \
    "Verdana Bold" \
    "Verdana Italic" \
    "Verdana" \
    "Verdana Bold Italic" \
    "VL Gothic" \
    "VL PGothic" \
    )

DEVANAGARI_FONTS=( \
    "FreeSans" \
    "Chandas" \
    "Kalimati" \
    "Uttara" \
    "Lucida Sans" \
    "gargi Medium" \
    "Lohit Devanagari" \
    "Arial Unicode MS Bold" \
    "Ascender Uni" \
    "Noto Sans Devanagari Bold" \
    "Noto Sans Devanagari" \
    "Samyak Devanagari Medium" \
    "Sarai" \
    "Saral LT Bold" \
    "Saral LT Light" \
    "Nakula" \
    "Sahadeva" \
    "Samanata" \
    "Santipur OT Medium" \
    )

KANNADA_FONTS=( \
    "Kedage Bold" \
    "Kedage Italic" \
    "Kedage" \
    "Kedage Bold Italic" \
    "Mallige Bold" \
    "Mallige Italic" \
    "Mallige" \
    "Mallige Bold Italic" \
    "Arial Unicode MS" \
    "Arial Unicode MS Bold" \
    "Ascender Uni" \
    "cheluvi Medium" \
    "Noto Sans Kannada Bold" \
    "Noto Sans Kannada" \
    "Lohit Kannada" \
    "Tunga" \
    "Tunga Bold" \
    )

TELUGU_FONTS=( \
    "Pothana2000" \
    "Vemana2000" \
    "Lohit Telugu" \
    "Arial Unicode MS Bold" \
    "Ascender Uni" \
    "Dhurjati" \
    "Gautami Bold" \
    "Gidugu" \
    "Gurajada" \
    "Lakki Reddy" \
    "Mallanna" \
    "Mandali" \
    "NATS" \
    "NTR" \
    "Noto Sans Telugu Bold" \
    "Noto Sans Telugu" \
    "Peddana" \
    "Ponnala" \
    "Ramabhadra" \
    "Ravi Prakash" \
    "Sree Krushnadevaraya" \
    "Suranna" \
    "Suravaram" \
    "Tenali Ramakrishna" \
    "Gautami" \
    )

TAMIL_FONTS=( \
    "TAMu_Kadambri" \
    "TAMu_Kalyani" \
    "TAMu_Maduram" \
    "TSCu_Paranar" \
    "TSCu_Times" \
    "TSCu_Paranar Bold" \
    "FreeSans" \
    "FreeSerif" \
    "Lohit Tamil" \
    "Arial Unicode MS Bold" \
    "Ascender Uni" \
    "Droid Sans Tamil Bold" \
    "Droid Sans Tamil" \
    "Karla Tamil Inclined Bold Italic" \
    "Karla Tamil Inclined Italic" \
    "Karla Tamil Upright Bold" \
    "Karla Tamil Upright" \
    "Noto Sans Tamil Bold" \
    "Noto Sans Tamil" \
    "Noto Sans Tamil UI Bold" \
    "Noto Sans Tamil UI" \
    "TSCu_Comic Normal" \
    "Lohit Tamil Classical" \
    )

THAI_FONTS=( \
    "FreeSerif" \
    "FreeSerif Italic" \
    "Garuda" \
    "Norasi" \
    "Lucida Sans Typewriter" \
    "Lucida Sans" \
    "Garuda Oblique" \
    "Norasi Oblique" \
    "Norasi Italic" \
    "Garuda Bold" \
    "Norasi Bold" \
    "Lucida Sans Typewriter Bold" \
    "Lucida Sans Semi-Bold" \
    "Garuda Bold Oblique" \
    "Norasi Bold Italic" \
    "Norasi Bold Oblique" \
    "AnuParp LT Thai" \
    "Arial Unicode MS Bold" \
    "Arial Unicode MS" \
    "Ascender Uni" \
    "Loma" \
    "Noto Serif Thai Bold" \
    "Noto Serif Thai" \
    "Purisa Light" \
    "Sirichana LT Bold" \
    "Sirichana LT" \
    "Sukothai LT Bold" \
    "Sukothai LT" \
    "UtSaHaGumm LT Thai" \
    "Tahoma" \
    )

KOREAN_FONTS=( \
    "Arial Unicode MS" \
    "Arial Unicode MS Bold" \
    "Baekmuk Batang Patched" \
    "Baekmuk Batang" \
    "Baekmuk Dotum" \
    "Baekmuk Gulim" \
    "Baekmuk Headline" \
    )

CHI_SIM_FONTS=( \
    "AR PL UKai CN" \
    "AR PL UMing Patched Light" \
    "Arial Unicode MS" \
    "Arial Unicode MS Bold" \
    "WenQuanYi Zen Hei Medium" \
    )

CHI_TRA_FONTS=( \
    "AR PL UKai TW" \
    "AR PL UMing TW MBE Light" \
    "AR PL UKai Patched" \
    "AR PL UMing Patched Light" \
    "Arial Unicode MS" \
    "Arial Unicode MS Bold" \
    "WenQuanYi Zen Hei Medium" \
    )

JPN_FONTS=( \
    "TakaoExGothic" \
    "TakaoExMincho" \
    "TakaoGothic" \
    "TakaoMincho" \
    "TakaoPGothic" \
    "TakaoPMincho" \
    "VL Gothic" \
    "VL PGothic" \
    "Noto Sans Japanese Bold" \
    "Noto Sans Japanese Light" \
    )

RUSSIAN_FONTS=( \
    "Arial Bold" \
    "Arial Bold Italic" \
    "Arial Italic" \
    "Arial" \
    "Courier New Bold" \
    "Courier New Bold Italic" \
    "Courier New Italic" \
    "Courier New" \
    "Times New Roman, Bold" \
    "Times New Roman, Bold Italic" \
    "Times New Roman, Italic" \
    "Times New Roman," \
    "Georgia Bold" \
    "Georgia Italic" \
    "Georgia" \
    "Georgia Bold Italic" \
    "Trebuchet MS Bold" \
    "Trebuchet MS Bold Italic" \
    "Trebuchet MS Italic" \
    "Trebuchet MS" \
    "Verdana Bold" \
    "Verdana Italic" \
    "Verdana" \
    "Verdana Bold Italic" \
    "DejaVu Serif" \
    "DejaVu Serif Oblique" \
    "DejaVu Serif Bold" \
    "DejaVu Serif Bold Oblique" \
    "Lucida Bright" \
    "FreeSerif Bold" \
    "FreeSerif Bold Italic" \
    "DejaVu Sans Ultra-Light" \
    )

GREEK_FONTS=( \
    "Arial Unicode MS" \
    "Arial Unicode MS Bold" \
    "DejaVu Sans Mono" \
    "DejaVu Sans Mono Oblique" \
    "DejaVu Sans Mono Bold" \
    "DejaVu Sans Mono Bold Oblique" \
    "DejaVu Serif" \
    "DejaVu Serif Semi-Condensed" \
    "DejaVu Serif Oblique" \
    "DejaVu Serif Bold" \
    "DejaVu Serif Bold Oblique" \
    "DejaVu Serif Bold Semi-Condensed" \
    "FreeSerif Bold" \
    "FreeSerif Bold Italic" \
    "FreeSerif Italic" \
    "FreeSerif" \
    "GentiumAlt" \
    "GentiumAlt Italic" \
    "Linux Biolinum O Bold" \
    "Linux Biolinum O" \
    "Linux Libertine O Bold" \
    "Linux Libertine O" \
    "Linux Libertine O Bold Italic" \
    "Linux Libertine O Italic" \
    "Palatino Linotype Bold" \
    "Palatino Linotype Bold Italic" \
    "Palatino Linotype Italic" \
    "Palatino Linotype" \
    "UmePlus P Gothic" \
    "VL PGothic" \
    )

ANCIENT_GREEK_FONTS=( \
    "GFS Artemisia" \
    "GFS Artemisia Bold" \
    "GFS Artemisia Bold Italic" \
    "GFS Artemisia Italic" \
    "GFS Bodoni" \
    "GFS Bodoni Bold" \
    "GFS Bodoni Bold Italic" \
    "GFS Bodoni Italic" \
    "GFS Didot" \
    "GFS Didot Bold" \
    "GFS Didot Bold Italic" \
    "GFS Didot Italic" \
    "GFS DidotClassic" \
    "GFS Neohellenic" \
    "GFS Neohellenic Bold" \
    "GFS Neohellenic Bold Italic" \
    "GFS Neohellenic Italic" \
    "GFS Philostratos" \
    "GFS Porson" \
    "GFS Pyrsos" \
    "GFS Solomos" \
    )

ARABIC_FONTS=( \
    "Arabic Transparent Bold" \
    "Arabic Transparent" \
    "Arab" \
    "Arial Unicode MS Bold" \
    "Arial Unicode MS" \
    "ASVCodar LT Bold" \
    "ASVCodar LT Light" \
    "Badiya LT Bold" \
    "Badiya LT" \
    "Badr LT Bold" \
    "Badr LT" \
    "Dimnah" \
    "Frutiger LT Arabic Bold" \
    "Frutiger LT Arabic" \
    "Furat" \
    "Hassan LT Bold" \
    "Hassan LT Light" \
    "Jalal LT Bold" \
    "Jalal LT Light" \
    "Midan Bold" \
    "Midan" \
    "Mitra LT Bold" \
    "Mitra LT Light" \
    "Palatino LT Arabic" \
    "Palatino Sans Arabic Bold" \
    "Palatino Sans Arabic" \
    "Simplified Arabic Bold" \
    "Simplified Arabic" \
    "Times New Roman, Bold" \
    "Times New Roman," \
    "Traditional Arabic Bold" \
    "Traditional Arabic" \
    )

HEBREW_FONTS=( \
    "Arial Bold" \
    "Arial Bold Italic" \
    "Arial Italic" \
    "Arial" \
    "Courier New Bold" \
    "Courier New Bold Italic" \
    "Courier New Italic" \
    "Courier New" \
    "Ergo Hebrew Semi-Bold" \
    "Ergo Hebrew Semi-Bold Italic" \
    "Ergo Hebrew" \
    "Ergo Hebrew Italic" \
    "Really No 2 LT W2G Light" \
    "Really No 2 LT W2G Light Italic" \
    "Really No 2 LT W2G Medium" \
    "Really No 2 LT W2G Medium Italic" \
    "Really No 2 LT W2G Semi-Bold" \
    "Really No 2 LT W2G Semi-Bold Italic" \
    "Really No 2 LT W2G Ultra-Bold" \
    "Really No 2 LT W2G Ultra-Bold Italic" \
    "Times New Roman, Bold" \
    "Times New Roman, Bold Italic" \
    "Times New Roman, Italic" \
    "Times New Roman," \
    "Lucida Sans" \
    "Tahoma" \
    )

BENGALI_FONTS=( \
    "Bangla Medium" \
    "Lohit Bengali" \
    "Mukti Narrow" \
    "Mukti Narrow Bold" \
    "Jamrul Medium Semi-Expanded" \
    "Likhan Medium" \
    "Arial Unicode MS Bold" \
    "Ascender Uni" \
    "FreeSans" \
    "FreeSans Oblique" \
    "FreeSerif" \
    "FreeSerif Italic" \
    "Noto Sans Bengali Bold" \
    "Noto Sans Bengali" \
    "Ani" \
    "Lohit Assamese" \
    "Lohit Bengali" \
    "Mitra Mono" \
    )

KYRGYZ_FONTS=( \
    "Arial" \
    "Arial Bold" \
    "Arial Italic" \
    "Arial Bold Italic" \
    "Courier New" \
    "Courier New Bold" \
    "Courier New Italic" \
    "Courier New Bold Italic" \
    "Times New Roman," \
    "Times New Roman, Bold" \
    "Times New Roman, Bold Italic" \
    "Times New Roman, Italic" \
    "DejaVu Serif" \
    "DejaVu Serif Oblique" \
    "DejaVu Serif Bold" \
    "DejaVu Serif Bold Oblique" \
    "Lucida Bright" \
    "FreeSerif Bold" \
    "FreeSerif Bold Italic" \
    )

PERSIAN_FONTS=( \
    "Amiri Bold Italic" \
    "Amiri Bold" \
    "Amiri Italic" \
    "Amiri" \
    "Andale Sans Arabic Farsi" \
    "Arial Unicode MS" \
    "Arial Unicode MS Bold" \
    "Lateef" \
    "Lucida Bright" \
    "Lucida Sans Oblique" \
    "Lucida Sans Semi-Bold" \
    "Lucida Sans" \
    "Lucida Sans Typewriter Bold" \
    "Lucida Sans Typewriter Oblique" \
    "Lucida Sans Typewriter" \
    "Scheherazade" \
    "Tahoma" \
    "Times New Roman," \
    "Times New Roman, Bold" \
    "Times New Roman, Bold Italic" \
    "Times New Roman, Italic" \
    "Yakout Linotype Bold" \
    "Yakout Linotype" \
    )

AMHARIC_FONTS=( \
    "Abyssinica SIL"
    "Droid Sans Ethiopic Bold" \
    "Droid Sans Ethiopic" \
    "FreeSerif" \
    "Noto Sans Ethiopic Bold" \
    "Noto Sans Ethiopic" \
    )

ARMENIAN_FONTS=( \
    "Arial Unicode MS" \
    "Arial Unicode MS Bold" \
    "Ascender Uni" \
    "FreeMono" \
    "FreeMono Italic" \
    "FreeSans" \
    "FreeSans Bold" \
    "FreeSans Oblique" \
    )

BURMESE_FONTS=( \
    "Myanmar Sans Pro" \
    "Noto Sans Myanmar Bold" \
    "Noto Sans Myanmar" \
    "Padauk Bold" \
    "Padauk" \
    "TharLon" \
    )

NORTH_AMERICAN_ABORIGINAL_FONTS=( \
    "Aboriginal Sans" \
    "Aboriginal Sans Bold Italic" \
    "Aboriginal Sans Italic" \
    "Aboriginal Sans Bold" \
    "Aboriginal Serif Bold" \
    "Aboriginal Serif Bold Italic" \
    "Aboriginal Serif Italic" \
    "Aboriginal Serif" \
    )

GEORGIAN_FONTS=( \
    "Arial Unicode MS Bold" \
    "Arial Unicode MS" \
    "BPG Algeti GPL\&GNU" \
    "BPG Chveulebrivi GPL\&GNU" \
    "BPG Courier GPL\&GNU" \
    "BPG Courier S GPL\&GNU" \
    "BPG DejaVu Sans 2011 GNU-GPL" \
    "BPG Elite GPL\&GNU" \
    "BPG Excelsior GPL\&GNU" \
    "BPG Glaho GPL\&GNU" \
    "BPG Gorda GPL\&GNU" \
    "BPG Ingiri GPL\&GNU" \
    "BPG Mrgvlovani Caps GNU\&GPL" \
    "BPG Mrgvlovani GPL\&GNU" \
    "BPG Nateli Caps GPL\&GNU Light" \
    "BPG Nateli Condenced GPL\&GNU Light" \
    "BPG Nateli GPL\&GNU Light" \
    "BPG Nino Medium Cond GPL\&GNU" \
    "BPG Nino Medium GPL\&GNU Medium" \
    "BPG Sans GPL\&GNU" \
    "BPG Sans Medium GPL\&GNU" \
    "BPG Sans Modern GPL\&GNU" \
    "BPG Sans Regular GPL\&GNU" \
    "BPG Serif GPL\&GNU" \
    "BPG Serif Modern GPL\&GNU" \
    "FreeMono" \
    "FreeMono Bold Italic" \
    "FreeSans" \
    "FreeSerif" \
    "FreeSerif Bold" \
    "FreeSerif Bold Italic" \
    "FreeSerif Italic" \
    )

OLD_GEORGIAN_FONTS=( \
    "Arial Unicode MS Bold" \
    "Arial Unicode MS" \
    "BPG Algeti GPL\&GNU" \
    "BPG Courier S GPL\&GNU" \
    "BPG DejaVu Sans 2011 GNU-GPL" \
    "BPG Elite GPL\&GNU" \
    "BPG Excelsior GPL\&GNU" \
    "BPG Glaho GPL\&GNU" \
    "BPG Ingiri GPL\&GNU" \
    "BPG Mrgvlovani Caps GNU\&GPL" \
    "BPG Mrgvlovani GPL\&GNU" \
    "BPG Nateli Caps GPL\&GNU Light" \
    "BPG Nateli Condenced GPL\&GNU Light" \
    "BPG Nateli GPL\&GNU Light" \
    "BPG Nino Medium Cond GPL\&GNU" \
    "BPG Nino Medium GPL\&GNU Medium" \
    "BPG Sans GPL\&GNU" \
    "BPG Sans Medium GPL\&GNU" \
    "BPG Sans Modern GPL\&GNU" \
    "BPG Sans Regular GPL\&GNU" \
    "BPG Serif GPL\&GNU" \
    "BPG Serif Modern GPL\&GNU" \
    "FreeSans" \
    "FreeSerif" \
    "FreeSerif Bold" \
    "FreeSerif Bold Italic" \
    "FreeSerif Italic" \
    )

KHMER_FONTS=( \
    "Khmer OS" \
    "Khmer OS System" \
    "Khmer OS Battambang" \
    "Khmer OS Bokor" \
    "Khmer OS Content" \
    "Khmer OS Fasthand" \
    "Khmer OS Freehand" \
    "Khmer OS Metal Chrieng" \
    "Khmer OS Muol Light" \
    "Khmer OS Muol Pali" \
    "Khmer OS Muol" \
    "Khmer OS Siemreap" \
    "Noto Sans Bold" \
    "Noto Sans" \
    "Noto Serif Khmer Bold" \
    "Noto Serif Khmer Light" \
    )

KURDISH_FONTS=( \
    "Amiri Bold Italic" \
    "Amiri Bold" \
    "Amiri Italic" \
    "Amiri" \
    "Arial Unicode MS" \
    "Arial Unicode MS Bold" \
    "Lateef" \
    "Lucida Bright" \
    "Lucida Sans Oblique" \
    "Lucida Sans Semi-Bold" \
    "Lucida Sans" \
    "Lucida Sans Typewriter Bold" \
    "Lucida Sans Typewriter Oblique" \
    "Lucida Sans Typewriter" \
    "Scheherazade" \
    "Tahoma" \
    "Times New Roman," \
    "Times New Roman, Bold" \
    "Times New Roman, Bold Italic" \
    "Times New Roman, Italic" \
    "Unikurd Web" \
    "Yakout Linotype Bold" \
    "Yakout Linotype" \
    )

LAOTHIAN_FONTS=( \
    "Phetsarath OT" \
    "Arial Unicode MS" \
    "Arial Unicode MS Bold" \
    "Ascender Uni" \
    "Dhyana Bold" \
    "Dhyana" \
    "Lao Muang Don" \
    "Lao Muang Khong" \
    "Lao Sans Pro" \
    "Noto Sans Lao Bold" \
    "Noto Sans Lao" \
    "Noto Sans Lao UI Bold" \
    "Noto Sans Lao UI" \
    "Noto Serif Lao Bold" \
    "Noto Serif Lao" \
    "Phetsarath Bold" \
    "Phetsarath" \
    "Souliyo Unicode" \
)

GUJARATI_FONTS=( \
    "Lohit Gujarati" \
    "Rekha Medium" \
    "Samyak Gujarati Medium" \
    "aakar Medium" \
    "padmaa Bold" \
    "padmaa Medium" \
    "Arial Unicode MS" \
    "Arial Unicode MS Bold" \
    "Ascender Uni" \
    "FreeSans" \
    "Noto Sans Gujarati Bold" \
    "Noto Sans Gujarati" \
    "Shruti" \
    "Shruti Bold" \
    )

MALAYALAM_FONTS=( \
    "AnjaliOldLipi" \
    "Arial Unicode MS" \
    "Arial Unicode MS Bold" \
    "Ascender Uni" \
    "Dyuthi" \
    "FreeSerif" \
    "Kalyani" \
    "Kartika" \
    "Kartika Bold" \
    "Lohit Malayalam" \
    "Meera" \
    "Noto Sans Malayalam Bold" \
    "Noto Sans Malayalam" \
    "Rachana" \
    "Rachana_w01" \
    "RaghuMalayalam" \
    "suruma" \
    )

ORIYA_FONTS=( \
    "Arial Unicode MS" \
    "Arial Unicode MS Bold" \
    "Ascender Uni" \
    "ori1Uni Medium" \
    "Samyak Oriya Medium" \
    "Lohit Oriya" \
    )

PUNJABI_FONTS=( \
    "Arial Unicode MS" \
    "Arial Unicode MS Bold" \
    "Ascender Uni" \
    "Saab" \
    "Lohit Punjabi" \
    "Noto Sans Gurmukhi" \
    "Noto Sans Gurmukhi Bold" \
    "FreeSans" \
    "FreeSans Bold" \
    "FreeSerif" \
    )

SINHALA_FONTS=( \
    "Noto Sans Sinhala Bold" \
    "Noto Sans Sinhala" \
    "OCRUnicode" \
    "Yagpo" \
    "LKLUG" \
    "FreeSerif" \
    )

SYRIAC_FONTS=( \
    "East Syriac Adiabene" \
    "East Syriac Ctesiphon" \
    "Estrangelo Antioch" \
    "Estrangelo Edessa" \
    "Estrangelo Midyat" \
    "Estrangelo Nisibin" \
    "Estrangelo Quenneshrin" \
    "Estrangelo Talada" \
    "Estrangelo TurAbdin" \
    "Serto Batnan Bold" \
    "Serto Batnan" \
    "Serto Jerusalem Bold" \
    "Serto Jerusalem Italic" \
    "Serto Jerusalem" \
    "Serto Kharput" \
    "Serto Malankara" \
    "Serto Mardin Bold" \
    "Serto Mardin" \
    "Serto Urhoy Bold" \
    "Serto Urhoy" \
    "FreeSans" \
    )

THAANA_FONTS=( \
    "FreeSerif" \
    )

TIBETAN_FONTS=( \
    "Arial Unicode MS" \
    "Arial Unicode MS Bold" \
    "Ascender Uni" \
    "DDC Uchen" \
    "Jomolhari" \
    "Kailasa" \
    "Kokonor" \
    "Tibetan Machine Uni" \
    "TibetanTsugRing" \
    "Yagpo" \
    )

# The following fonts will be rendered vertically in phase I.
VERTICAL_FONTS=( \
    "TakaoExGothic" \ # for jpn
    "TakaoExMincho" \ # for jpn
    "AR PL UKai Patched" \ # for chi_tra
    "AR PL UMing Patched Light" \ # for chi_tra
    "Baekmuk Batang Patched" \ # for kor
    )

# Set language-specific values for several global variables, including
#   ${TEXT_CORPUS}
#      holds the text corpus file for the language, used in phase F
#   ${FONTS[@]}
#      holds a sequence of applicable fonts for the language, used in
#      phase F & I. only set if not already set, i.e. from command line
#   ${TRAINING_DATA_ARGUMENTS}
#      non-default arguments to the training_data program used in phase T
#   ${FILTER_ARGUMENTS} -
#      character-code-specific filtering to distinguish between scripts
#      (eg. CJK) used by filter_borbidden_characters in phase F
#   ${WORDLIST2DAWG_ARGUMENTS}
#      specify fixed length dawg generation for non-space-delimited lang
# TODO(dsl): We can refactor these into functions that assign FONTS,
# TEXT_CORPUS, etc. separately.
set_lang_specific_parameters() {
  local lang=$1
  # The default text location is now given directly from the language code.
  TEXT_CORPUS="${FLAGS_webtext_prefix}/${lang}.corpus.txt"
  FILTER_ARGUMENTS=""
  WORDLIST2DAWG_ARGUMENTS=""
  # These dawg factors represent the fraction of the corpus not covered by the
  # dawg, and seem like reasonable defaults, but the optimal value is likely
  # to be highly corpus-dependent, as well as somewhat language-dependent.
  # Number dawg factor is the fraction of all numeric strings that are not
  # covered, which is why it is higher relative to the others.
  PUNC_DAWG_FACTOR=
  NUMBER_DAWG_FACTOR=0.125
  WORD_DAWG_FACTOR=0.05
  BIGRAM_DAWG_FACTOR=0.015
  TRAINING_DATA_ARGUMENTS=""
  FRAGMENTS_DISABLED="y"
  RUN_SHAPE_CLUSTERING=0
  AMBIGS_FILTER_DENOMINATOR="100000"
  LEADING="32"
  MEAN_COUNT="40"  # Default for latin script.
  # Language to mix with the language for maximum accuracy. Defaults to eng.
  # If no language is good, set to the base language.
  MIX_LANG="eng"

  case ${lang} in
    # Latin languages.
    enm ) TEXT2IMAGE_EXTRA_ARGS=" --ligatures"   # Add ligatures when supported
          test -z "$FONTS" && FONTS=( "${EARLY_LATIN_FONTS[@]}" );;
    frm ) TEXT_CORPUS="${FLAGS_webtext_prefix}/fra.corpus.txt"
          # Make long-s substitutions for Middle French text
          FILTER_ARGUMENTS="--make_early_language_variant=fra"
          TEXT2IMAGE_EXTRA_ARGS=" --ligatures"   # Add ligatures when supported.
          test -z "$FONTS" && FONTS=( "${EARLY_LATIN_FONTS[@]}" );;
    frk ) TEXT_CORPUS="${FLAGS_webtext_prefix}/deu.corpus.txt"
          test -z "$FONTS" && FONTS=( "${FRAKTUR_FONTS[@]}" );;
    ita_old )
          TEXT_CORPUS="${FLAGS_webtext_prefix}/ita.corpus.txt"
          # Make long-s substitutions for Early Italian text
          FILTER_ARGUMENTS="--make_early_language_variant=ita"
          TEXT2IMAGE_EXTRA_ARGS=" --ligatures"   # Add ligatures when supported.
          test -z "$FONTS" && FONTS=( "${EARLY_LATIN_FONTS[@]}" );;
    lat )
          test -z "$EXPOSURES" && EXPOSURES="-3 -2 -1 0 1 2 3"
          test -z "$FONTS" && FONTS=( "${NEOLATIN_FONTS[@]}" ) ;;
    spa_old )
          TEXT_CORPUS="${FLAGS_webtext_prefix}/spa.corpus.txt"
          # Make long-s substitutions for Early Spanish text
          FILTER_ARGUMENTS="--make_early_language_variant=spa"
          TEXT2IMAGE_EXTRA_ARGS=" --ligatures"  # Add ligatures when supported.
          test -z "$FONTS" && FONTS=( "${EARLY_LATIN_FONTS[@]}" );;
    srp_latn )
          TEXT_CORPUS=${FLAGS_webtext_prefix}/srp.corpus.txt ;;
    vie ) TRAINING_DATA_ARGUMENTS+=" --infrequent_ratio=10000"
          test -z "$FONTS" && FONTS=( "${VIETNAMESE_FONTS[@]}" ) ;;
    # Highly inflective languages get a bigger dawg size.
    # TODO(rays) Add more here!
    hun ) WORD_DAWG_SIZE=1000000 ;;
    pol ) WORD_DAWG_SIZE=1000000 ;;

    # Latin with default treatment.
    afr ) ;;
    aze ) ;;
    bos ) ;;
    cat ) ;;
    ceb ) ;;
    ces ) PUNC_DAWG_FACTOR=0.004 ;;
    cym ) ;;
    dan ) ;;
    deu ) WORD_DAWG_FACTOR=0.125 ;;
    eng ) WORD_DAWG_FACTOR=0.03 ;;
    epo ) ;;
    est ) ;;
    eus ) ;;
    fil ) ;;
    fin ) ;;
    fra ) WORD_DAWG_FACTOR=0.08 ;;
    gle ) ;;
    glg ) ;;
    hat ) ;;
    hrv ) ;;
    ind ) ;;
    isl ) ;;
    ita ) ;;
    jav ) ;;
    lav ) ;;
    lit ) ;;
    mlt ) ;;
    msa ) ;;
    nld ) WORD_DAWG_FACTOR=0.02 ;;
    nor ) ;;
    por ) ;;
    ron ) ;;
    slk ) ;;
    slv ) ;;
    spa ) ;;
    sqi ) ;;
    swa ) ;;
    swe ) ;;
    tgl ) ;;
    tur ) ;;
    uzb ) ;;
    zlm ) ;;

    # Special code for performing language-id that is trained on
    # EFIGS+Latin+Vietnamese text with regular + fraktur fonts.
    lat_lid )
          TEXT_CORPUS=${FLAGS_webtext_prefix}/lat_lid.corpus.txt
          TRAINING_DATA_ARGUMENTS+=" --infrequent_ratio=10000"
          GENERATE_WORD_BIGRAMS=0
          # Strip unrenderable words as not all fonts will render the extended
          # latin symbols found in Vietnamese text.
          WORD_DAWG_SIZE=1000000
          test -z "$FONTS" && FONTS=( "${EARLY_LATIN_FONTS[@]}" );;

    # Cyrillic script-based languages. It is bad to mix Latin with Cyrillic.
    rus ) test -z "$FONTS" && FONTS=( "${RUSSIAN_FONTS[@]}" )
          MIX_LANG="rus"
          NUMBER_DAWG_FACTOR=0.05
          WORD_DAWG_SIZE=1000000 ;;
    aze_cyrl | bel | bul | kaz | mkd | srp | tgk | ukr | uzb_cyrl )
          MIX_LANG="${lang}"
          test -z "$FONTS" && FONTS=( "${RUSSIAN_FONTS[@]}" ) ;;

    # Special code for performing Cyrillic language-id that is trained on
    # Russian, Serbian, Ukranian, Belarusian, Macedonian, Tajik and Mongolian
    # text with the list of Russian fonts.
    cyr_lid )
          TEXT_CORPUS=${FLAGS_webtext_prefix}/cyr_lid.corpus.txt
          TRAINING_DATA_ARGUMENTS+=" --infrequent_ratio=10000"
          GENERATE_WORD_BIGRAMS=0
          WORD_DAWG_SIZE=1000000
          test -z "$FONTS" && FONTS=( "${RUSSIAN_FONTS[@]}" );;

    # South Asian scripts mostly have a lot of different graphemes, so trim
    # down the MEAN_COUNT so as not to get a huge amount of text.
    asm | ben )
          MEAN_COUNT="15"
          WORD_DAWG_FACTOR=0.15
          test -z "$FONTS" && FONTS=( "${BENGALI_FONTS[@]}" ) ;;
    bih | hin | mar | nep | san )
          MEAN_COUNT="15"
          WORD_DAWG_FACTOR=0.15
          test -z "$FONTS" && FONTS=( "${DEVANAGARI_FONTS[@]}" ) ;;
    bod ) MEAN_COUNT="15"
          WORD_DAWG_FACTOR=0.15
          test -z "$FONTS" && FONTS=( "${TIBETAN_FONTS[@]}" ) ;;
    dzo )
          WORD_DAWG_FACTOR=0.01
          test -z "$FONTS" && FONTS=( "${TIBETAN_FONTS[@]}" ) ;;
    guj ) MEAN_COUNT="15"
          WORD_DAWG_FACTOR=0.15
          test -z "$FONTS" && FONTS=( "${GUJARATI_FONTS[@]}" ) ;;
    kan ) MEAN_COUNT="15"
          WORD_DAWG_FACTOR=0.15
          TRAINING_DATA_ARGUMENTS+=" --no_newline_in_output"
          TEXT2IMAGE_EXTRA_ARGS=" --char_spacing=0.5"
          test -z "$FONTS" && FONTS=( "${KANNADA_FONTS[@]}" ) ;;
    mal ) MEAN_COUNT="15"
          WORD_DAWG_FACTOR=0.15
          TRAINING_DATA_ARGUMENTS+=" --no_newline_in_output"
          TEXT2IMAGE_EXTRA_ARGS=" --char_spacing=0.5"
          test -z "$FONTS" && FONTS=( "${MALAYALAM_FONTS[@]}" ) ;;
    ori )
          WORD_DAWG_FACTOR=0.01
          test -z "$FONTS" && FONTS=( "${ORIYA_FONTS[@]}" ) ;;
    pan ) MEAN_COUNT="15"
          WORD_DAWG_FACTOR=0.01
          test -z "$FONTS" && FONTS=( "${PUNJABI_FONTS[@]}" ) ;;
    sin ) MEAN_COUNT="15"
          WORD_DAWG_FACTOR=0.01
          test -z "$FONTS" && FONTS=( "${SINHALA_FONTS[@]}" ) ;;
    tam ) MEAN_COUNT="30"
          WORD_DAWG_FACTOR=0.15
          TRAINING_DATA_ARGUMENTS+=" --no_newline_in_output"
          TEXT2IMAGE_EXTRA_ARGS=" --char_spacing=0.5"
          test -z "$FONTS" && FONTS=( "${TAMIL_FONTS[@]}" ) ;;
    tel ) MEAN_COUNT="15"
          WORD_DAWG_FACTOR=0.15
          TRAINING_DATA_ARGUMENTS+=" --no_newline_in_output"
          TEXT2IMAGE_EXTRA_ARGS=" --char_spacing=0.5"
          test -z "$FONTS" && FONTS=( "${TELUGU_FONTS[@]}" ) ;;

    # SouthEast Asian scripts.
    khm ) MEAN_COUNT="15"
          WORD_DAWG_FACTOR=0.15
          TRAINING_DATA_ARGUMENTS+=" --infrequent_ratio=10000"
          test -z "$FONTS" && FONTS=( "${KHMER_FONTS[@]}" ) ;;
    lao ) MEAN_COUNT="15"
          WORD_DAWG_FACTOR=0.15
          TRAINING_DATA_ARGUMENTS+=" --infrequent_ratio=10000"
          test -z "$FONTS" && FONTS=( "${LAOTHIAN_FONTS[@]}" ) ;;
    mya ) MEAN_COUNT="12"
          WORD_DAWG_FACTOR=0.15
          TRAINING_DATA_ARGUMENTS+=" --infrequent_ratio=10000"
          test -z "$FONTS" && FONTS=( "${BURMESE_FONTS[@]}" ) ;;
    tha ) MEAN_COUNT="30"
          WORD_DAWG_FACTOR=0.01
          TRAINING_DATA_ARGUMENTS+=" --infrequent_ratio=10000"
          FILTER_ARGUMENTS="--segmenter_lang=tha"
          TRAINING_DATA_ARGUMENTS+=" --no_space_in_output --desired_bigrams="
          AMBIGS_FILTER_DENOMINATOR="1000"
          LEADING=48
          test -z "$FONTS" && FONTS=( "${THAI_FONTS[@]}" ) ;;

    # CJK
    chi_sim )
          MEAN_COUNT="15"
          PUNC_DAWG_FACTOR=0.015
          WORD_DAWG_FACTOR=0.015
          GENERATE_WORD_BIGRAMS=0
          TRAINING_DATA_ARGUMENTS+=" --infrequent_ratio=10000"
          TRAINING_DATA_ARGUMENTS+=" --no_space_in_output --desired_bigrams="
          FILTER_ARGUMENTS="--charset_filter=chi_sim --segmenter_lang=chi_sim"
          test -z "$FONTS" && FONTS=( "${CHI_SIM_FONTS[@]}" ) ;;
    chi_tra )
          MEAN_COUNT="15"
          WORD_DAWG_FACTOR=0.015
          GENERATE_WORD_BIGRAMS=0
          TRAINING_DATA_ARGUMENTS+=" --infrequent_ratio=10000"
          TRAINING_DATA_ARGUMENTS+=" --no_space_in_output --desired_bigrams="
          FILTER_ARGUMENTS="--charset_filter=chi_tra --segmenter_lang=chi_tra"
          test -z "$FONTS" && FONTS=( "${CHI_TRA_FONTS[@]}" ) ;;
    jpn ) MEAN_COUNT="15"
          WORD_DAWG_FACTOR=0.015
          GENERATE_WORD_BIGRAMS=0
          TRAINING_DATA_ARGUMENTS+=" --infrequent_ratio=10000"
          TRAINING_DATA_ARGUMENTS+=" --no_space_in_output --desired_bigrams="
          FILTER_ARGUMENTS="--charset_filter=jpn --segmenter_lang=jpn"
          test -z "$FONTS" && FONTS=( "${JPN_FONTS[@]}" ) ;;
    kor ) MEAN_COUNT="20"
          WORD_DAWG_FACTOR=0.015
          NUMBER_DAWG_FACTOR=0.05
          TRAINING_DATA_ARGUMENTS+=" --infrequent_ratio=10000"
          TRAINING_DATA_ARGUMENTS+=" --desired_bigrams="
          GENERATE_WORD_BIGRAMS=0
          FILTER_ARGUMENTS="--charset_filter=kor --segmenter_lang=kor"
          test -z "$FONTS" && FONTS=( "${KOREAN_FONTS[@]}" ) ;;

    # Middle-Eastern scripts.
    ara ) test -z "$FONTS" && FONTS=( "${ARABIC_FONTS[@]}" ) ;;
    div ) test -z "$FONTS" && FONTS=( "${THAANA_FONTS[@]}" ) ;;
    fas | pus | snd | uig | urd )
          test -z "$FONTS" && FONTS=( "${PERSIAN_FONTS[@]}" ) ;;
    heb | yid )
          NUMBER_DAWG_FACTOR=0.05
          WORD_DAWG_FACTOR=0.08
          test -z "$FONTS" && FONTS=( "${HEBREW_FONTS[@]}" ) ;;
    syr ) test -z "$FONTS" && FONTS=( "${SYRIAC_FONTS[@]}" ) ;;

    # Other scripts.
    amh | tir)
          test -z "$FONTS" && FONTS=( "${AMHARIC_FONTS[@]}" ) ;;
    chr ) test -z "$FONTS" && FONTS=( "${NORTH_AMERICAN_ABORIGINAL_FONTS[@]}" \
                  "Noto Sans Cherokee" \
                ) ;;
    ell )
          NUMBER_DAWG_FACTOR=0.05
          WORD_DAWG_FACTOR=0.08
          test -z "$FONTS" && FONTS=( "${GREEK_FONTS[@]}" ) ;;
    grc )
          test -z "$EXPOSURES" && EXPOSURES="-3 -2 -1 0 1 2 3"
          test -z "$FONTS" && FONTS=( "${ANCIENT_GREEK_FONTS[@]}" ) ;;
    hye ) test -z "$FONTS" && FONTS=( "${ARMENIAN_FONTS[@]}" ) ;;
    iku ) test -z "$FONTS" && FONTS=( "${NORTH_AMERICAN_ABORIGINAL_FONTS[@]}" ) ;;
    kat)  test -z "$FONTS" && FONTS=( "${GEORGIAN_FONTS[@]}" ) ;;
    kat_old)
          TEXT_CORPUS="${FLAGS_webtext_prefix}/kat.corpus.txt"
          test -z "$FONTS" && FONTS=( "${OLD_GEORGIAN_FONTS[@]}" ) ;;
    kir ) test -z "$FONTS" && FONTS=( "${KYRGYZ_FONTS[@]}" )
          TRAINING_DATA_ARGUMENTS=" --infrequent_ratio=100" ;;
    kur ) test -z "$FONTS" && FONTS=( "${KURDISH_FONTS[@]}" ) ;;

    *) err_exit "Error: ${lang} is not a valid language code"
  esac
  if [[ ${FLAGS_mean_count} -gt 0 ]]; then
    TRAINING_DATA_ARGUMENTS+=" --mean_count=${FLAGS_mean_count}"
  elif [[ ! -z ${MEAN_COUNT} ]]; then
    TRAINING_DATA_ARGUMENTS+=" --mean_count=${MEAN_COUNT}"
  fi
  # Default to Latin fonts if none have been set
  test -z "$FONTS" && FONTS=( "${LATIN_FONTS[@]}" )

  # Default to 0 exposure if it hasn't been set
  test -z "$EXPOSURES" && EXPOSURES=0
}

#=============================================================================
# END of Language specific info
#=============================================================================
