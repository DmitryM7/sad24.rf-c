#include <Arduino.h>
#include <MemoryFree.h>
#include <mstr.h>

mstr::mstr() {
};

void mstr::trim(char* iStr,char* iPattern) {
     if (strlen(iPattern) !=1 ) {
        return;
     };
  return trim(iStr,iPattern[0]);
}
void mstr::trim(char* iStr,char iPattern) {
  unsigned int vNotZeroPosition;

 
  if (iStr[0]==iPattern) {
    for (int vI = 0; vI < strlen(iStr); vI++) {
      if (iStr[vI]!=iPattern) {
           vNotZeroPosition = vI;
           break;      
      };
    };

    for (int vI = 0; vI <strlen(iStr); vI++) {
      iStr[vI] = iStr[vI + vNotZeroPosition];
    };
  };

  for (int vI = strlen(iStr) - 1; vI>-1; vI--) {
            
    if (iStr[vI] != iPattern) {
       break;
    };
    iStr[vI] = 0;
  };
}

bool mstr::isEqual(char* iStr1,char* iStr2) {
   return strcmp(iStr1,iStr2) == 0;
};

int mstr::numEntries(char* iStr,char* iDelimiter) {
  if (strlen(iDelimiter)!=1) { 
     return false;
  };
  return numEntries(iStr,iDelimiter[0]);
}
int mstr::numEntries(char* iStr,char iDelimiter) {
  return numEntries(iStr,iDelimiter,1,strlen(iStr));
};

int mstr::numEntries(char* iStr,char* iDelimiter,int iStartPosition,int iEndPosition) {
  if (strlen(iDelimiter)!=1) { 
     return false;
  };
  return numEntries(iStr,iDelimiter[0],iStartPosition,iEndPosition);
};
int mstr::numEntries(char* iStr,char iDelimiter,int iStartPosition,int iEndPosition) {
    unsigned int vNumEntries = 1;

    for (unsigned int vI = iStartPosition; vI < iEndPosition; vI++) {
        if (iStr[vI]==iDelimiter) {
           vNumEntries++;
        };
    };

  return vNumEntries;
};


bool mstr::entry(unsigned int iField,char* iStr,char* iDelimiter,char* oRes) {
  if (strlen(iDelimiter)!=1) { 
     return false;
  };
  return entry(iField,iStr,iDelimiter[0],20,oRes);
}

bool mstr::entry(unsigned int iField,char* iStr,char* iDelimiter,int iMaxEntrySize,char* oRes) {
  if (strlen(iDelimiter)!=1) { 
     return false;
  };

 return entry(iField,iStr,iDelimiter[0],iMaxEntrySize,oRes);
}

bool mstr::entry(unsigned int iField,char* iStr,char iDelimiter,int iMaxEntrySize,char* oRes) {

   unsigned int vPrevPos = 0, vCurrPos = -1, vCurrField = 1, vStrLength = strlen(iStr);
   bool vEndRase = false;
   int vDiffPos;

   for (unsigned int vI = 0; vI < vStrLength; vI++) {

     if ( iStr[vI]==iDelimiter || ( vEndRase = (vI == vStrLength - 1)) ) {
      vPrevPos = vCurrPos;
      vCurrPos = vI;
      /*************************************************************
       * Если последний символ в строке разделитель, то его        *
       * исключаем из итогового значения.                          *
       *************************************************************/
      vDiffPos = max(vCurrPos - vPrevPos - (vEndRase && iStr[vI] != iDelimiter ? 0 : 1),0);
      
      if (vCurrField == iField) {

        if (vDiffPos > iMaxEntrySize) { return false; };
        strncpy(oRes, iStr + vPrevPos + 1,vDiffPos);
        oRes[vDiffPos]='\0';
        return true;

      };
      
      vCurrField++;
    };
   };
   
   
   return false;   
}


int mstr::indexOf(char* iStr, char* iPattern) {
  return indexOf(iStr,iPattern,0);
};

int mstr::indexOf(char* iStr, char* iPattern,int iStartPosition) {
      char* pch;

      pch = strstr(iStr + iStartPosition,iPattern);


      if (pch!=NULL && pch >= iStr) {      
        return pch - iStr;
      };

  return -1;
};


bool mstr::begins(char* iStr,char* iPattern) {
  char* p1;
  p1 = strstr(iStr,iPattern);

  if (p1 >= iStr) {
    return p1 - iStr == 0;
  };
  
  return false;
};

void mstr::substr(char* iStr,int iStartPos,int iLength,char* oStr) {
  strncpy(oStr,iStr + iStartPos,iLength);
  oStr[iLength]='\0';
};

void mstr::leftShift2(char* iStr,unsigned int iShift) { 
  unsigned int vStrLen=strlen(iStr);
  memmove(iStr,iStr+iShift,vStrLen-iShift);
  memset(iStr + (vStrLen-iShift-1),0,iShift);
}

void mstr::_emptyBuffer(char* oBuf,size_t iSize) {
 memset(oBuf,'\0',iSize);
}
