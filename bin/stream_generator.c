/* ----------------------------------------------------------------- */
/*           The HMM-Based Speech Synthesis Engine "hts_engine API"  */
/*           developed by HTS Working Group                          */
/*           http://hts-engine.sourceforge.net/                      */
/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2001-2011  Nagoya Institute of Technology          */
/*                           Department of Computer Science          */
/*                                                                   */
/*                2001-2008  Tokyo Institute of Technology           */
/*                           Interdisciplinary Graduate School of    */
/*                           Science and Engineering                 */
/*                                                                   */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/* - Redistributions of source code must retain the above copyright  */
/*   notice, this list of conditions and the following disclaimer.   */
/* - Redistributions in binary form must reproduce the above         */
/*   copyright notice, this list of conditions and the following     */
/*   disclaimer in the documentation and/or other materials provided */
/*   with the distribution.                                          */
/* - Neither the name of the HTS working group nor the names of its  */
/*   contributors may be used to endorse or promote products derived */
/*   from this software without specific prior written permission.   */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/* ----------------------------------------------------------------- */
/*                                                   */
/*         Single Stream Parameter Generator         */
/*  2013   Beijing Institute of Technology           */
/*  2013   Idiap Research Institute                  */
/*         Xingyu Na | Hsing-Yu Na                   */
/* ------------------------------------------------- */

#ifndef HTS_ENGINE_C
#define HTS_ENGINE_C

#ifdef __cplusplus
#define HTS_ENGINE_C_START extern "C" {
#define HTS_ENGINE_C_END   }
#else
#define HTS_ENGINE_C_START
#define HTS_ENGINE_C_END
#endif                          /* __CPLUSPLUS */

HTS_ENGINE_C_START;

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include "HTS_engine.h"

/* Usage: output usage */
void Usage(void)
{
   HTS_show_copyright(stderr);
   fprintf(stderr, "\n");
   fprintf(stderr, "stream_generator - The HMM-based Single Stream Parameter Generator \"stream_generator\"\n");
   fprintf(stderr, "\n");
   fprintf(stderr, "  usage:\n");
   fprintf(stderr, "       stream_generator [ options ] [ infile ] \n");
   fprintf(stderr, "  options:                                                                   [  def][ min--max]\n");
   fprintf(stderr, "    -td tree       : decision tree files for state duration                  [  N/A]\n");
   fprintf(stderr, "    -ta tree       : decision tree files for acoustic model                  [  N/A]\n");
   fprintf(stderr, "    -md pdf        : model files for state duration                          [  N/A]\n");
   fprintf(stderr, "    -ma pdf        : model files for acoustic model                          [  N/A]\n");
   fprintf(stderr, "    -da win        : window files for calculation delta of acoustic model    [  N/A]\n");
   fprintf(stderr, "    -od s          : filename of output label with duration                  [  N/A]\n");
   fprintf(stderr, "    -oa s          : filename of output acoustic parameter sequence          [  N/A]\n");
   fprintf(stderr, "    -ot s          : filename of output trace information                    [  N/A]\n");
   fprintf(stderr, "    -vp            : use phoneme alignment for duration                      [  N/A]\n");
   fprintf(stderr, "    -i  i f1 .. fi : enable interpolation & specify number(i),coefficient(f) [    1][   1-- ]\n");
   fprintf(stderr, "    -s  i          : sampling frequency                                      [16000][   1--48000]\n");
   fprintf(stderr, "    -p  i          : frame period (point)                                    [   80][   1--]\n");
   fprintf(stderr, "    -r  f          : speech speed rate                                       [  1.0][ 0.0--10.0]\n");
   fprintf(stderr, "    -u  f          : two-space MSD threshold                                 [  0.5][ 0.0--1.0]\n");
   fprintf(stderr, "    -ea tree       : decision tree files for GV of acoustic model            [  N/A]\n");
   fprintf(stderr, "    -ca pdf        : filenames of GV for acoustic model                      [  N/A]\n");
   fprintf(stderr, "    -ja f          : weight of GV for acoustic model                         [  1.0][ 0.0--2.0]\n");
   fprintf(stderr, "    -k  tree       : GV switch                                               [  N/A]\n");
   fprintf(stderr, "  infile:\n");
   fprintf(stderr, "    label file\n");
   fprintf(stderr, "  note:\n");
   fprintf(stderr, "    option '-d' may be repeated to use multiple delta parameters.\n");
   fprintf(stderr, "    generated sequences are saved in natural endian, binary (float) format.\n");
   fprintf(stderr, "\n");

   exit(0);
}

/* Error: output error message */
void Error(const int error, char *message, ...)
{
   va_list arg;

   fflush(stdout);
   fflush(stderr);

   if (error > 0)
      fprintf(stderr, "\nError: ");
   else
      fprintf(stderr, "\nWarning: ");

   va_start(arg, message);
   vfprintf(stderr, message, arg);
   va_end(arg);

   fflush(stderr);

   if (error > 0)
      exit(error);
}

/* GetNumInterp: get number of speakers for interpolation from argv */
int GetNumInterp(int argc, char **argv_search)
{
   int num_interp = 1;
   while (--argc) {
      if (**++argv_search == '-') {
         if (*(*argv_search + 1) == 'i') {
            num_interp = atoi(*++argv_search);
            if (num_interp < 1) {
               num_interp = 1;
            }
            --argc;
         }
      }
   }
   return (num_interp);
}

int main(int argc, char **argv)
{
   int i;
   char *labfn = NULL;
   HTS_File *durfp = NULL, *acsfp = NULL, *tracefp = NULL;

   /* number of speakers for interpolation */
   int num_interp = 0;
   double *rate_interp = NULL;

   /* file names of models */
   char **fn_ms_dur;
   char **fn_ms_acs;
   /* number of each models for interpolation */
   int num_ms_dur = 0, num_ms_acs = 0;

   /* file names of trees */
   char **fn_ts_dur;
   char **fn_ts_acs;
   /* number of each trees for interpolation */
   int num_ts_dur = 0, num_ts_acs = 0;

   /* file names of windows */
   char **fn_ws_acs;
   int num_ws_acs = 0;

   /* file names of global variance */
   char **fn_ms_gva = NULL;
   int num_ms_gva = 0;

   /* file names of global variance trees */
   char **fn_ts_gva = NULL;
   int num_ts_gva = 0;

   /* file name of global variance switch */
   char *fn_gv_switch = NULL;

   /* global parameter */
   int sampling_rate = 16000;
   int fperiod = 80;
   double msd_threshold = 0.5;
   double gv_weight_acs = 1.0;

   HTS_Boolean phoneme_alignment = FALSE;
   double speech_speed = 1.0;

   /* engine */
   HTS_Engine engine;

   /* parse command line */
   if (argc == 1)
      Usage();

   /* delta window handler for mel-cepstrum */
   fn_ws_acs = (char **) calloc(argc, sizeof(char *));

   /* prepare for interpolation */
   num_interp = GetNumInterp(argc, argv);
   rate_interp = (double *) calloc(num_interp, sizeof(double));
   for (i = 0; i < num_interp; i++)
      rate_interp[i] = 1.0;

   fn_ms_dur = (char **) calloc(num_interp, sizeof(char *));
   fn_ms_acs = (char **) calloc(num_interp, sizeof(char *));
   fn_ts_dur = (char **) calloc(num_interp, sizeof(char *));
   fn_ts_acs = (char **) calloc(num_interp, sizeof(char *));
   fn_ms_gva = (char **) calloc(num_interp, sizeof(char *));
   fn_ts_gva = (char **) calloc(num_interp, sizeof(char *));

   /* read command */
   while (--argc) {
      if (**++argv == '-') {
         switch (*(*argv + 1)) {
         case 'v':
            switch (*(*argv + 2)) {
            case 'p':
               phoneme_alignment = TRUE;
               break;
            default:
               Error(1, "hts_engine: Invalid option '-v%c'.\n", *(*argv + 2));
            }
            break;
         case 't':
            switch (*(*argv + 2)) {
            case 'd':
               fn_ts_dur[num_ts_dur++] = *++argv;
               break;
            case 'a':
               fn_ts_acs[num_ts_acs++] = *++argv;
               break;
            default:
               Error(1, "hts_engine: Invalid option '-t%c'.\n", *(*argv + 2));
            }
            --argc;
            break;
         case 'm':
            switch (*(*argv + 2)) {
            case 'd':
               fn_ms_dur[num_ms_dur++] = *++argv;
               break;
            case 'a':
               fn_ms_acs[num_ms_acs++] = *++argv;
               break;
            default:
               Error(1, "hts_engine: Invalid option '-m%c'.\n", *(*argv + 2));
            }
            --argc;
            break;
         case 'd':
            switch (*(*argv + 2)) {
            case 'a':
               fn_ws_acs[num_ws_acs++] = *++argv;
               break;
            default:
               Error(1, "hts_engine: Invalid option '-d%c'.\n", *(*argv + 2));
            }
            --argc;
            break;
         case 'o':
            switch (*(*argv + 2)) {
            case 'd':
               durfp = HTS_fopen(*++argv, "wt");
               break;
            case 'a':
               acsfp = HTS_fopen(*++argv, "wb");
               break;
            case 't':
               tracefp = HTS_fopen(*++argv, "wt");
               break;
            default:
               Error(1, "hts_engine: Invalid option '-o%c'.\n", *(*argv + 2));
            }
            --argc;
            break;
         case 'h':
            Usage();
            break;
         case 's':
            sampling_rate = atoi(*++argv);
            --argc;
            break;
         case 'p':
            fperiod = atoi(*++argv);
            --argc;
            break;
         case 'r':
            speech_speed = atof(*++argv);
            --argc;
            break;
         case 'u':
            msd_threshold = atof(*++argv);
            --argc;
            break;
         case 'i':
            ++argv;
            argc--;
            for (i = 0; i < num_interp; i++) {
               rate_interp[i] = atof(*++argv);
               argc--;
            }
            break;
         case 'e':
            switch (*(*argv + 2)) {
            case 'a':
               fn_ts_gva[num_ts_gva++] = *++argv;
               break;
            default:
               Error(1, "hts_engine: Invalid option '-e%c'.\n", *(*argv + 2));
            }
            --argc;
            break;
         case 'c':
            switch (*(*argv + 2)) {
            case 'a':
               fn_ms_gva[num_ms_gva++] = *++argv;
               break;
            default:
               Error(1, "hts_engine: Invalid option '-c%c'.\n", *(*argv + 2));
            }
            --argc;
            break;
         case 'j':
            switch (*(*argv + 2)) {
            case 'a':
               gv_weight_acs = atof(*++argv);
               break;
            default:
               Error(1, "hts_engine: Invalid option '-j%c'.\n", *(*argv + 2));
            }
            --argc;
            break;
         case 'k':
            fn_gv_switch = *++argv;
            --argc;
            break;
         default:
            Error(1, "hts_engine: Invalid option '-%c'.\n", *(*argv + 1));
         }
      } else {
         labfn = *argv;
      }
   }
   /* number of models,trees check */
   if (num_interp != num_ts_dur || num_interp != num_ts_acs || num_interp != num_ms_dur || num_interp != num_ms_acs) {
      Error(1, "hts_engine: specify %d models(trees) for each parameter.\n", num_interp);
   }

   /* initialize */
   HTS_Engine_initialize(&engine, 1);

   /* load duration model */
   HTS_Engine_load_duration_from_fn(&engine, fn_ms_dur, fn_ts_dur, num_interp);
   /* load acoustic model */
   HTS_Engine_load_parameter_from_fn(&engine, fn_ms_acs, fn_ts_acs, fn_ws_acs, 0, num_ws_acs, num_interp);
   /* load GV for acoustic model */
   if (num_interp == num_ms_gva) {
      if (num_ms_gva == num_ts_gva)
         HTS_Engine_load_gv_from_fn(&engine, fn_ms_gva, fn_ts_gva, 0, num_interp);
      else
         HTS_Engine_load_gv_from_fn(&engine, fn_ms_gva, NULL, 0, num_interp);
   }
   /* load GV switch */
   if (fn_gv_switch != NULL)
      HTS_Engine_load_gv_switch_from_fn(&engine, fn_gv_switch);

   /* set parameter */
   HTS_Engine_set_sampling_rate(&engine, sampling_rate);
   HTS_Engine_set_fperiod(&engine, fperiod);
   HTS_Engine_set_msd_threshold(&engine, 0, msd_threshold);      /* set voiced/unvoiced threshold for stream[1] */
   HTS_Engine_set_gv_weight(&engine, 0, gv_weight_acs);
   for (i = 0; i < num_interp; i++) {
      HTS_Engine_set_duration_interpolation_weight(&engine, i, rate_interp[i]);
      HTS_Engine_set_parameter_interpolation_weight(&engine, 0, i, rate_interp[i]);
   }
   if (num_interp == num_ms_gva)
      for (i = 0; i < num_interp; i++)
         HTS_Engine_set_gv_interpolation_weight(&engine, 0, i, rate_interp[i]);

   /* synthesis */
   HTS_Engine_load_label_from_fn(&engine, labfn);       /* load label file */
   if (phoneme_alignment)       /* modify label */
      HTS_Label_set_frame_specified_flag(&engine.label, TRUE);
   if (speech_speed != 1.0)     /* modify label */
      HTS_Label_set_speech_speed(&engine.label, speech_speed);
   HTS_Engine_create_sstream(&engine);  /* parse label and determine state duration */
   HTS_Engine_create_pstream(&engine);  /* generate speech parameter vector sequence */
   HTS_Engine_create_gstream(&engine);  /* synthesize speech */

   /* output */
   if (tracefp != NULL)
      HTS_Engine_save_information(&engine, tracefp);
   if (durfp != NULL)
      HTS_Engine_save_label(&engine, durfp);
   if (acsfp)
      HTS_Engine_save_generated_parameter(&engine, acsfp, 0);

   /* free */
   HTS_Engine_refresh(&engine);

   /* free memory */
   HTS_Engine_clear(&engine);
   free(rate_interp);
   free(fn_ws_acs);
   free(fn_ms_acs);
   free(fn_ms_dur);
   free(fn_ts_acs);
   free(fn_ts_dur);
   free(fn_ms_gva);
   free(fn_ts_gva);

   /* close files */
   if (durfp != NULL)
      HTS_fclose(durfp);
   if (acsfp != NULL)
      HTS_fclose(acsfp);
   if (tracefp != NULL)
      HTS_fclose(tracefp);

   return 0;
}

HTS_ENGINE_C_END;

#endif                          /* !HTS_ENGINE_C */
