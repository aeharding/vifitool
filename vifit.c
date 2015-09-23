/*
 *  VIFIT - VIdeo Flicker Investigation Tool
 *  Build: 4/27/2011
 *  Copyright (C) 2011  Alexander Harding
 *
 *  Roadmap:
 *  Build 3/4/2011      First public release
 *  Build 4/27/2011     Algorithm bug fix (minimal result impact)

 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    char    *inputFile;
    int     width;
    int     height;
    char    *outputFile;
    long    numFrames;
} cmdArgs_t;

static int parseArgs(int argc, char **argv, cmdArgs_t *args)
{
    int retVal = 0;

    // clear arguments
    memset( args, 0, sizeof (cmdArgs_t));

    while (argc > 0)
    {
        if ( strcmp(*argv, "-i") == 0 )
        {
            argv++;
            argc--;
            if (argc > 0)
            {
                args->inputFile = *argv++;
                argc--;
            }
            else
                retVal = 1;
        }
        else if ( strcmp(*argv, "-w") == 0 )
        {
            argv++;
            argc--;
            if (argc > 0)
            {
                args->width = atoi(*argv++);
                argc--;
            }
            else
                retVal = 1;
        }
        else if ( strcmp(*argv, "-h") == 0 )
        {
            argv++;
            argc--;
            if (argc > 0)
            {
                args->height = atoi(*argv++);
                argc--;
            }
            else
                retVal = 1;
        }
        else if ( strcmp(*argv, "-o") == 0 )
        {
            argv++;
            argc--;
            if (argc > 0)
            {
                args->outputFile = *argv++;
                argc--;
            }
            else
                retVal = 1;
        }
        else if ( strcmp(*argv, "-n") == 0 )
        {
            argv++;
            argc--;
            if (argc > 0)
            {
                args->numFrames = atoi(*argv++);
                argc--;
            }
            else
                retVal = 1;
        }
        else
        {
            // unknown argument
            retVal = 1;
            break;
        }
    }

    // check arguments for completeness
    if (args->numFrames == 0) retVal = 1;
    if (args->inputFile == NULL) retVal = 1;
    if (args->outputFile == NULL) retVal = 1;

    if ( retVal )
    {
        // there was an error parsing the command arguments
        printf("usage:   cmdline -i inputfile -w pixelwidth -h pixelheight -o outputfile[w/o extention] -n numFrames\n");
        printf("example: cmdline -i input.yuv -w 1024 -h 768 -o output -n 100\n");
        getchar();
        exit(0);
    }
    return retVal;
}


int main(int argc, char **argv)
{
    cmdArgs_t argsVal;
    cmdArgs_t *args = &argsVal;
    // discard first argument (the executable name)
    argc--;
    argv++;
    
    if (parseArgs(argc, argv, args))
    {
        // command line parsing failed - get out
        exit(1);
    }
    
    /* VARIABLE DECLARATION */

    int i , j , m , n;     //Loop variables        

    /* Constants for media */
    int WIDTH = args->width;
    int HEIGHT = args->height;
    int LRGH = HEIGHT - (HEIGHT/3);
    int LRGW = WIDTH - (WIDTH/3);
    int SMLH = HEIGHT / 3;
    int SMLW = WIDTH / 3;
    int THRESH = (SMLH*SMLW) / 4;
    
    unsigned long long size;        //Size of file
    long frm;               //Current frame
    int fail = 0;          //How many frame failures in video
    int Rfail = 0;         //How many failures (frames) in video (red)
    int fails=1;           //Total number of TOTAL defects (used for bullet points)
    int Rdefect = 0;       //How many total red defects in video
    int defect = 0;        //How many total lum defects in video

    long histstrt=0;        //First frame of defect
    long histfin=0;         //Final frame of defect
    long Rhiststrt=0;       //First frame of defect (red)
    long Rhistfin=0;        //Final frame of defect (red)
    double Rmagn = 0;      //Used to discover average magnitude of defect (red)
    double magn = 0;       //Used to discover average magnitude of defect

    /*Conversion variables for output summary [frames --> time]*/
    float milli;
    int seconds;
    int minutes;
    int hours;
    int totalseconds;


    double red=0,green=0,blue=0; //Temporary storage place for final RGB ==> Lum conversion
    int posfail[32] ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, negfail[32] ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, Rposfail[32] ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, Rnegfail[32] ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //Failure history variables
    unsigned char buffer[HEIGHT][WIDTH*3]; //Buffer for raw RGBRGBRGB form

    int satPrev[HEIGHT][WIDTH]; //Find saturated or not 0/1 matrix.
    int satCurr[HEIGHT][WIDTH]; //Find saturated or not 0/1 matrix.

    /* Dynamic memory allocations for frames */
    double** redCurr; //Red Previous frame
    redCurr = (double**) malloc(HEIGHT*sizeof(double*));
    for (i = 0; i < HEIGHT; i++)
        redCurr[i] = (double*) malloc(WIDTH*sizeof(double));
        
    double** redPrev; //Red Previous frame
    redPrev = (double**) malloc(HEIGHT*sizeof(double*));
    for (i = 0; i < HEIGHT; i++)
        redPrev[i] = (double*) malloc(WIDTH*sizeof(double));
        
    double** redDiff; //Red difference frame
    redDiff = (double**) malloc(HEIGHT*sizeof(double*));
    for (i = 0; i < HEIGHT; i++)
        redDiff[i] = (double*) malloc(WIDTH*sizeof(double));
        
    double** lumPrev; //Previous frame
    lumPrev = (double**) malloc(HEIGHT*sizeof(double*));
    for (i = 0; i < HEIGHT; i++)
        lumPrev[i] = (double*) malloc(WIDTH*sizeof(double));
        
    double** lumCurr;//Current frame
    lumCurr = (double**) malloc(HEIGHT*sizeof(double*));
    for (i = 0; i < HEIGHT; i++)
        lumCurr[i] = (double*) malloc(WIDTH*sizeof(double));
        
    double** lumDiff; //Difference frame
    lumDiff = (double**) malloc(HEIGHT*sizeof(double*));
    for (i = 0; i < HEIGHT; i++)
        lumDiff[i] = (double*) malloc(WIDTH*sizeof(double));
    
    /*File opening*/
    char strtemp[40];
    strcpy(strtemp,args->outputFile);
    FILE * output;
    output = fopen(strcat(strtemp,"_log.csv"), "w+");
    
    strcpy(strtemp,args->outputFile);
    FILE * outputsum;
    outputsum = fopen(strcat(strtemp,"_summary.txt"), "w+");
    
    FILE * input;
    input = fopen(args->inputFile, "rb");
    if (input==NULL) {printf ("File error: File doesn't exist. %s",args->inputFile); exit (1);}
    
    /* -1 frames calculation*/
    if (args->numFrames < -1)
    {
               printf("Incorrect amount of frames.");
               exit(1);
    }
    
    fseek(input, 0, SEEK_END); // seek to end of file
    size = ftell(input);
    size /= WIDTH*HEIGHT*3;
    if (args->numFrames == -1)
    {
        args->numFrames = size;
    }
    else
    {
        if (size > args->numFrames)
        {
            printf("Frame amount overflow: Set n frames to 0 to automatically scan whole file.");
            exit(1);
        }
    }
    fseek(input, 0, SEEK_SET); // seek back to beginning of file
    
    /* Initial user I/O */
    printf("VIFIT build 04-27-2011.\n\n");
    fprintf(output,"VIFIT build 04-27-2011.\n\n");
    
    fprintf(outputsum,"  _    _____________________________\n   \\  /    |   |______   |      |   \n    \\/   __|__ |       __|__    |   \n");
    fprintf(outputsum,"Build 04-27-2011 created by Alex Harding.\n\n");
    fprintf(outputsum,"Results of analysis for compliance with Guidance 2.3 of\nthe Web Content Accessibility Guidelines (WCAG) 2.0\n\n");
    
    printf("Input filename: %s\n",args->inputFile);
    fprintf(output,"Input filename: %s\n",args->inputFile);
    fprintf(outputsum,"Input filename: %s\n",args->inputFile);
    
    printf("W: %i & H: %i\n",WIDTH,HEIGHT);
    fprintf(output,"W: %i & H: %i\n",WIDTH,HEIGHT);
    fprintf(outputsum,"W: %i & H: %i\n",WIDTH,HEIGHT);
    
    printf("Output filename: | For CSV: %s_log.csv | For summary file: %s_summary.txt\n",args->outputFile,args->outputFile);
    fprintf(output,"Output filename:,Log: %s_log.csv,Summary file: %s_summary.txt\n",args->outputFile,args->outputFile);
    fprintf(outputsum,"Output filename: | For log: %s_log.txt | For summary file: %s_summary.txt\n",args->outputFile,args->outputFile);
    
    printf("Total frames to scan: %i\n",args->numFrames);
    fprintf(output,"Total frames to scan: %i\n",args->numFrames);
    fprintf(outputsum,"Total frames to scan: %i\n",args->numFrames);
    
    printf("\nScanning material...\n");
    printf("Anything over 1 is a failure.\n\n");
    printf("\t\tLum\tLum\tLum\tLum\tRed\tRed\tRed\tRed\n");
    printf("Frame #\t\t-to+\t+to-\tFrames\tFail?\t-to+\t+to-\tFrames\tFail?");
    
    fprintf(output,"\nScanning material...\n");
    fprintf(output,"Anything over 0 is a failure.\n\n");
    fprintf(output,"PrevFrame,CurrFrame,,Lum -to+,Lum +to-,Lum Frames,Lum fail?,,Red -to+,Red +to-,Red frames,Red fail?");

    /* FILE LOADING TO MATRIX */
    for (frm = 0; frm <= args->numFrames - 1; frm++) //Main loop
    {
        for (j = 0; j < HEIGHT; j++)
        {
            fread(buffer[j],WIDTH*3,1,input);
        }
    
        for (j = 0; j < HEIGHT; j++)
        {
            for (i = 0; i < WIDTH; i++)
            {
                satPrev[j][i] = 0;
            }
        }

        /* sRGB TO RELATIVE LUMINANCE */
        for (j = 0; j < HEIGHT; j++)
        {
            for (i = 0; i < (WIDTH*3); )
            {
                if ((double)buffer[j][i]/255 <= 0.03928) //R
                {
                    red = ((double)buffer[j][i]/255)/12.92;
                }
                else
                {
                    red = pow(((((double)buffer[j][i])/255+0.055)/1.055),2.4);
                }
                i++;
                if ((double)buffer[j][i]/255 <= 0.03928) //G
                {
                    green = ((double)buffer[j][i]/255)/12.92;
                }
                else
                {
                    green = pow(((((double)buffer[j][i])/255+0.055)/1.055),2.4);
                }
                i++;
                if ((double)buffer[j][i]/255 <= 0.03928) //B
                {
                    blue = ((double)buffer[j][i]/255)/12.92;
                }
                else
                {
                    blue = pow(((((double)buffer[j][i])/255+0.055)/1.055),2.4);
                }
                i++;
                
                lumPrev[j][i/3] = (0.2126*red)+(0.7152*green)+(0.0722*blue); //Luminance calc
                
                redCurr[j][i/3] = (red-green-blue)*320; //Red calcs ...
                if ((red)/(red+green+blue) >= 0.8)
                {
                    satPrev[j][i/3] = 1;
                }
                if(redCurr[j][i/3] < 0.0)
                {
                    redCurr[j][i/3] = 0.0;
                }
            }
        }
    
        /* ANALYSIS */
        if (frm > 0) //On first run-through, lumCurr doesn't have values yet.
        {
    
            /* THRESHOLD ADJUSTING before runthrough */
            for (j = 0; j < HEIGHT; j++)
            {
                for (i = 0; i < WIDTH; i++)
                {
                    lumDiff[j][i] = lumPrev[j][i] - lumCurr[j][i]; //Find difference frame
                    
                    if (lumPrev[j][i] <= lumCurr[j][i])
                    {
                        if(lumPrev[j][i] >= 0.8)
                        {
                            lumDiff[j][i] = 0.0;
                        }
                    }
                    else
                    {
                        if(lumCurr[j][i] >= 0.8)
                        {
                            lumDiff[j][i] = 0.0;
                        }
                    }
                    
                    redDiff[j][i] = redCurr[j][i] - redPrev[j][i]; //Red flash
                    
                    if(satPrev[j][i]==1 || satCurr[j][i]==1)
                    {
                        redDiff[j][i] = 0.0;
                    }
                }
            }
    
            for(n = 0; n < LRGH; n++) //Loop moving box on height.
            {
                //if (posfail[0]!=0 && negfail[0]!=0) break;
                for(m = 0; m < LRGW; m++) //Loop moving box on width.
                {
                    int poscount=0, negcount=0;
                    int Rposcount=0, Rnegcount=0;
                    for(j = n; j < n+SMLH; j++) //Analyze pixels in box.
                    {
                        for(i = m; i < m+SMLW; i++)
                        {
                            if(lumDiff[j][i] > 0.1) poscount++;
                            if(lumDiff[j][i] < -0.1) negcount++;
                            
                            if(redDiff[j][i] > 20) Rposcount++;
                            if(redDiff[j][i] < -20) Rnegcount++;
                        }
                    }
                    if(poscount > THRESH) posfail[0]++;
                    if(negcount > THRESH) negfail[0]++;
                    if(Rposcount > THRESH) Rposfail[0]++;
                    if(Rnegcount > THRESH) Rnegfail[0]++;
                    //if (posfail[0]!=0 && negfail[0]!=0) break;
                }
            }

            //Output:
            printf("\n#%i==>#%i ",frm,frm+1);
            printf("\t%.3f",(float)posfail[0]/(LRGH*LRGW));
            printf("\t%.3f",(float)negfail[0]/(LRGH*LRGW));
            
            fprintf(output,"\n%i,%i,",frm,frm+1);
            fprintf(output,"----,=%i/%i",posfail[0],(LRGH*LRGW));
            fprintf(output,",=%i/%i",negfail[0],(LRGH*LRGW));
            
            
        } //Close for frm > 0

        for (j = 0; j < HEIGHT; j++)
        {
            for (i = 0; i < WIDTH; i++)
            {
                lumCurr[j][i] = lumPrev[j][i];
                redPrev[j][i] = redCurr[j][i];
                satCurr[j][i] = satPrev[j][i];
            }
        }
        
        int failcount=0;
        int Rfailcount=0;
        for(i=0;i<30;i++) //Look for pairs in last second.
        {
            if((posfail[i]!=0) && (negfail[i+1]!=0)) failcount++;
            else
            if((negfail[i]!=0) && (posfail[i+1]!=0)) failcount++;
            
            if((Rposfail[i]!=0) && (Rnegfail[i+1]!=0)) Rfailcount++;
            else
            if((Rnegfail[i]!=0) && (Rposfail[i+1]!=0)) Rfailcount++;
        }
        
        if (frm > 0)
        {
            printf("\t%i",failcount);
            fprintf(output,",%i",failcount);
        }
        
        if (failcount > 3) //adds up pairs within last second.
        {
            printf("\tLUM");
            fprintf(output,",LUM,----");
            fail++;
            magn += (double)failcount;
            
            if (histfin > 0)
            {
                     histfin++;
            }
            else
            {
                histfin = frm;
                histstrt = frm;
                defect++;
            }
        }
        else
        {
            printf("\t");
            if(frm>0) fprintf(output,",,----");
            if (histfin > 0)
            {
                totalseconds = histstrt/30;
                hours = totalseconds/3600;
                minutes = (totalseconds/60) % 60;
                seconds = totalseconds % 60;
                milli = (histstrt % 30);
                fprintf(outputsum,"\n   %i.\tLuminance defect from\t%01i:%02i:%02i.%02.0f ",fails,hours,minutes,seconds,milli);
                
                histfin -= 1;
                totalseconds = histfin/30;
                hours = totalseconds/3600;
                minutes = (totalseconds/60) % 60;
                seconds = totalseconds % 60;
                milli = (histfin % 30);
                fprintf(outputsum,"to %01i:%02i:%02i.%02.0f ",hours,minutes,seconds,milli);
                
                magn /= (histfin-histstrt)+2;
                fprintf(outputsum,"with an average frame amount of %.2f.",magn);
                
                magn = 0.0;
                histstrt = 0;
                histfin = 0;
                fails++;
            }
        }
    
        if(frm > 0)
        {
               printf("\t%.3f",(float)Rposfail[0]/(LRGH*LRGW));
               fprintf(output,",=%i/%i",Rposfail[0],(LRGH*LRGW));
               
               printf("\t%.3f",(float)Rnegfail[0]/(LRGH*LRGW));
               fprintf(output,",=%i/%i",Rnegfail[0],(LRGH*LRGW));
               
               printf("\t%i",Rfailcount);
               fprintf(output,",%i",Rfailcount);
        }
    
        if (Rfailcount > 3) //adds up red pairs within last second.
        {
            printf("\tRED");
            fprintf(output,",RED");
            Rfail++;
            Rmagn += (double)Rfailcount;
            
            if (Rhistfin > 0)
            {
                Rhistfin++;
            }
            else
            {
                Rhistfin = frm;
                Rhiststrt = frm;
                Rdefect++;
            }
        }
        else
        {
            printf("\t");
            fprintf(output,",");
            if (Rhistfin > 0)
            {
                totalseconds = Rhiststrt/30;
                hours = totalseconds/3600;
                minutes = (totalseconds/60) % 60;
                seconds = totalseconds % 60;
                milli = (Rhiststrt % 30);
                fprintf(outputsum,"\n   %i.\tRed defect from \t%01i:%02i:%02i.%02.0f ",fails,hours,minutes,seconds,milli);
                
                Rhistfin -= 1;
                totalseconds = Rhistfin/30;
                hours = totalseconds/3600;
                minutes = (totalseconds/60) % 60;
                seconds = totalseconds % 60;
                milli = (Rhistfin % 30);
                fprintf(outputsum,"to %01i:%02i:%02i.%02.0f ",hours,minutes,seconds,milli);
                
                Rmagn /= (Rhistfin-Rhiststrt)+2;
                fprintf(outputsum,"with an average frame amount of %.2f.",Rmagn);
                
                Rmagn = 0.0;
                Rhiststrt = 0;
                Rhistfin = 0;
                fails++;
            }
        }
                     
        for(i = 31;i > 0;i--) //Shift Posfail values over. Youngest to oldest.
        {
            posfail[i] = posfail[i-1];
            negfail[i] = negfail[i-1];
            
            Rposfail[i] = Rposfail[i-1];
            Rnegfail[i] = Rnegfail[i-1];
        }
        posfail[0] = 0;
        negfail[0] = 0;
        
        Rposfail[0] = 0;
        Rnegfail[0] = 0;
    
    } //Close for FRAMES

    printf("\n\nAnalysis complete.");
    fprintf(output,"\n\nAnalysis complete.");
    fprintf(outputsum,"\n\nAnalysis complete.");
    if(defect > 0 || Rdefect > 0)
    {
        fprintf(outputsum,"\nContent FAILS ");
    }
    else
    {
        fprintf(outputsum,"\nContent PASSES ");
    }
    fprintf(outputsum,"with %i total luminance defects over %i total frames and %i total red defects over %i total frames.",defect,fail,Rdefect,Rfail);
    
    fclose(input);
    fclose(output);
    fclose(outputsum);
    
    return(0);
}

