/****************************************************************************\
Copyright (c) 2002, NVIDIA Corporation.

NVIDIA Corporation("NVIDIA") supplies this software to you in
consideration of your agreement to the following terms, and your use,
installation, modification or redistribution of this NVIDIA software
constitutes acceptance of these terms.  If you do not agree with these
terms, please do not use, install, modify or redistribute this NVIDIA
software.

In consideration of your agreement to abide by the following terms, and
subject to these terms, NVIDIA grants you a personal, non-exclusive
license, under NVIDIA's copyrights in this original NVIDIA software (the
"NVIDIA Software"), to use, reproduce, modify and redistribute the
NVIDIA Software, with or without modifications, in source and/or binary
forms; provided that if you redistribute the NVIDIA Software, you must
retain the copyright notice of NVIDIA, this notice and the following
text and disclaimers in all such redistributions of the NVIDIA Software.
Neither the name, trademarks, service marks nor logos of NVIDIA
Corporation may be used to endorse or promote products derived from the
NVIDIA Software without specific prior written permission from NVIDIA.
Except as expressly stated in this notice, no other rights or licenses
express or implied, are granted by NVIDIA herein, including but not
limited to any patent rights that may be infringed by your derivative
works or by other works in which the NVIDIA Software may be
incorporated. No hardware is licensed hereunder. 

THE NVIDIA SOFTWARE IS BEING PROVIDED ON AN "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, WARRANTIES OR CONDITIONS OF TITLE,
NON-INFRINGEMENT, MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
ITS USE AND OPERATION EITHER ALONE OR IN COMBINATION WITH OTHER
PRODUCTS.

IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT,
INCIDENTAL, EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, LOST PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) OR ARISING IN ANY WAY
OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION OF THE
NVIDIA SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT,
TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF
NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\****************************************************************************/
//
// compile.h
//

#if !defined(__COMPILE_H)
#define __COMPILE_H 1

int InitCPPStruct(void);

typedef struct Options_Rec{
    const char *profileString;
    int ErrorMode;
    int Quiet;
	
    // Debug The Compiler options:
    int DumpAtomTable;
} Options;

#define MAX_IF_NESTING  64
struct CPPStruct_Rec {
    // Public members
    SourceLoc *pLastSourceLoc;  // Set at the start of each statement by the tree walkers
    Options options;            // Compile options and parameters

    // Private members
    SourceLoc lastSourceLoc;

    // Scanner data:

    SourceLoc *tokenLoc;        // Source location of most recent token seen by the scanner
    int mostRecentToken;        // Most recent token seen by the scanner
    InputSrc *currentInput;
    int previous_token;
    int pastFirstStatement;     // used to make sure that #version is the first statement seen in the file, if present
    
	void *pC;                   // storing the parseContext of the compile object in cpp.  
     
    // Private members:
    SourceLoc ltokenLoc;
	int ifdepth;                //current #if-#else-#endif nesting in the cpp.c file (pre-processor)    
    int elsedepth[MAX_IF_NESTING];//Keep a track of #if depth..Max allowed is 64.
    int elsetracker;            //#if-#else and #endif constructs...Counter.
    const char *ErrMsg;
    int CompileError;           //Indicate compile error when #error, #else,#elif mismatch.

    //
    // Globals used to communicate between PaParseStrings() and yy_input()and 
    // also across the files.(gen_glslang.cpp and scanner.c)
    //
    int PaWhichStr;             // which string we're parsing
    const int* PaStrLen;        // array of lengths of the PaArgv strings
    int PaArgc;                 // count of strings in the array
    const char* const* PaArgv;  // our array of strings to parse    
    unsigned int tokensBeforeEOF : 1;
};

#endif // !defined(__COMPILE_H)
