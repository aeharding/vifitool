/* Pseudocode(c-improv'd) for program developed
to screen for flickering in videos.

-- VIFIT --

Media needs to be 1024*768 @ 30FPS.
Alexander Harding | 10/10/10 */

unsigned char aaPrev[768][1024], aaCurr[768][1024]; //Initialize variable matrixes for current and previous frame.
int int aaCurraaDiff [768][1024];
bool posfail[31] = {FALSE,};
bool negfail[31] = {FALSE,};
int i, j, m, n;
read(aaPrev); //Determine method: FFMPEG read or direct input.
filter(aaPrev); //WCAG 2.0 exception compliance.

while(TRUE) //Video still playing.

{
    read(aaCurr); //Matrix of luma values [0-255].
    filter(aaCurr); //WCAG 2.0 exception compliance.
    for(j=0;j<768;j++) //Loop creating difference matrix for height.
        for(i=0;i<1024;i++) //Loop creating difference matrix for width.
            aaDiff[j][i] = aaCurr[j][i] - aaPrev[j][i]; //Find difference frame: Buffers frame diffs ahead of time.
    for(n=0;n<768-256;n++) //Loop moving box on height.
    {
        for(m=0;1024-341;m++) //Loop moving box on width.
            // ^ Later make above dynamic to frame size. ^
        {
            int poscount=0, negcount=0, failcount=0; //Variables used for the specific frame.
            for(j=0;j<256;j++) //
                for(i=0;341;i++)
                {
                    if(aaDiff[n+j][m+i] > 26) poscount++;
                    if(aaDiff[n+j][m+i] < -26) negcount++;
                }

            if(poscount > (341*256)/4) posfail[30]=TRUE;
            if(negcount > (341*256)/4) negfail[30]=TRUE;
        }
    }

     
    /*Evaluate posfail + negfail amount for:
    1. Transition Pairs
    2. Count transition pairs
    */
    for(i=0;i<=30;i++) //Look for pairs in last second.
    {
        if(posfail[i] && negfail[i+1]) failcount++;
        else
        if(negfail[i] && posfail[i+1]) failcount++;
    }
    if (failcount > 3) //adds up pairs within last second.
    {
        //[Display and output warning here.]
    }


    for(i=30;i>0;i--) //Shift Posfail values over for last second.
    {
        posfail[i+1] = posfail[i-1];
        negfail[i+1] = negfail[i-1];
    }

    aaPrev = aaCurr;
}
