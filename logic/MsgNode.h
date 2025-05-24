#pragma once
class CSession;
class LogicSystem;

class MsgNode {
  friend class CSession;
  friend class LogicSystem;

public:
  MsgNode(short len);
  ~MsgNode();
  void clear();

protected:
  short _curLen;
  short _totalLen;
  char *_data;
};

class RecvNode : public MsgNode {
public:
  RecvNode(short len, short msgID = -1);
  short getMsgID() const;

private:
  short _msgID;
};

class SendNode : public MsgNode {
public:
  SendNode(const char *msg, short len, short msgID);
  short getMsgID() const;

private:
  short _msgID;
};