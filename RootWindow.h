#pragma once
#include "ControlPane.h"
#include "CloudRender.h"
/*スレッド関数の引数データ構造体*/
typedef struct _thread_arg {
  int thread_no;
	int *data;
} thread_arg_t;

class CRootWindow
{
public:
	CRootWindow();
	~CRootWindow(void);
	void Renders();
	thread_arg_t ftarg;
	thread_arg_t rtarg;
};

