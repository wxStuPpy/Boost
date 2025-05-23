#include <iostream>
#include "const.h"

class MsgNode
{
public:
    MsgNode(short len);
    ~MsgNode();
    void clear();

protected:
    short _curLen;
    short _totalLen;
    char *_data;
};

class RecvNode : public MsgNode
{
public:
    RecvNode(short len, short msgId);

private:
    short _msgId;
};

class SendNode : public MsgNode
{
public:
    SendNode(const char *msg, short len, short msgId);

private:
    short _msgId;
};