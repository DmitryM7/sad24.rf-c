#ifndef mstr_h
#define mstr_h

class mstr
{

  public:
    mstr();
    bool entry(unsigned int iField,char* iStr,char  iDelimiter,int iMaxEntrySize,char* oFieldValue);
    bool entry(unsigned int iField,char* iStr,char* iDelimiter,int iMaxEntrySize,char* oFieldValue);
    bool entry(unsigned int iField,char* iStr,char* iDelimiter,char* oFieldValue);


    int numEntries(char* iStr,char iDelimiter,int iStartPosition,int iEndPosition);
    int numEntries(char* iStr,char* iDelimiter,int iStartPosition,int iEndPosition);
    int numEntries(char* iStr,char iDelimiter);
    int numEntries(char* iStr,char* iDelimiter);


    void trim(char* iStr,char iPattern);
    void trim(char* iStr,char* iPattern);
    bool isEqual(char* iStr1,char* iStr2);

    int indexOf(char* iStr, char* iPattern);
    int indexOf(char* iStr, char* iPattern,int iStartPosition);

    bool begins(char* iStr,char* iPattern);
    void substr(char* iStr,int iStartPos,int iLength,char* oStr);

    void leftShift2(char* iStr,unsigned int iShift);

};
#endif