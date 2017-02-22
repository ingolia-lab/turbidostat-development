#ifndef _controller_h
#define _controller_h 1

class Controller {
  public:
    virtual int begin(void);
    virtual int loop(void);

    virtual void manualSetParams(void);

    static const int loopMsec = 1000;
};

#endif /* !defined(_controller_h) */
