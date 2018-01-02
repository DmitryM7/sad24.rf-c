#include <Arduino.h>
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

bool mstr::entry(unsigned int iField,char* iStr,char iDelimiter,char* oRes) {
  return entry(iField,iStr,iDelimiter,20,oRes);
}

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
  return entry(iField,iStr,iDelimiter,iMaxEntrySize,0,strlen(iStr),oRes);
};

bool mstr::entry(unsigned int iField,char* iStr,char* iDelimiter,int iMaxEntrySize,int iStartPosition,int iEndPosition,char* oRes) {

  if (strlen(iDelimiter)!=1) { 
     return false;
  };

  return entry(iField,iStr,iDelimiter[0],iMaxEntrySize,iStartPosition,iEndPosition,oRes);
};

bool mstr::entry(unsigned int iField,
                 char* iStr,
                 char iDelimiter,
                 int iMaxEntrySize,
                 int iStartPosition,
                 int iEndPosition,
                 char* oRes) {
   int vDelimiterCurrPos=iEndPosition,
       vDelimiterPrevPos=iStartPosition-1,
       vDelimiter=1;
   char vRes[100];

   /******************************************************** 
    * Находим вхождение разделителя                        *
    * с номером iField. По-умолчанию, предполагаем, что    *
    * разделителей нет.                                    *
    ********************************************************/
   for (unsigned int vI=iStartPosition; vI<iEndPosition; vI++) {

        if (iStr[vI]==iDelimiter) {
           if (vDelimiter==iField) {
               vDelimiterCurrPos=vI - 1;
               break;
           } else {
               vDelimiterPrevPos=vI; 
               vDelimiter++;
           };
        };
   };




   if (vDelimiterCurrPos - vDelimiterPrevPos > iMaxEntrySize) {
         return false;      
   };

    /********************************************************
     * Теперь в обратную сторону до следующего делителя     *
     * набираем нужные символы.                             *
     ********************************************************/

   vRes[vDelimiterCurrPos-vDelimiterPrevPos]='\0';

   for (int vI=vDelimiterCurrPos;vI>vDelimiterPrevPos;vI--) {
        vRes[vI-vDelimiterPrevPos-1]=iStr[vI];
   };
   strcpy(oRes,vRes);
   return true;   
}

int mstr::indexOf(char* iStr, char* iPattern) {
  return indexOf(iStr,iPattern,0);
};

int mstr::indexOf(char* iStr, char* iPattern,int iStartPosition) {
   char* pch;
   pch = strstr(iStr + iStartPosition,iPattern);

   if (pch >= iStr) {
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
