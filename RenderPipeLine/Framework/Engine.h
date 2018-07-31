/////////////////////////////////////////////////////////////////////////////////
/// Copyright (C), 2017, xizaixuan. All rights reserved.
/// \brief   引擎控制类
/// \author  xizaixuan
/// \date    2017-07
/////////////////////////////////////////////////////////////////////////////////
#ifndef Engine_H_
#define Engine_H_

#include <Windows.h>
#include "..\Utility\Singleton.h"

class Engine : public Singleton<Engine>
{
	SINGLETON_DEFINE(Engine)
private:
	Engine();
	~Engine();

public:
	/// \brief 初始化
	void init(HINSTANCE hInstance, int nCmdShow, int width, int height);

	/// \brief 资源销毁
	void destroy();

	/// \brief 更新
	void update(float dt);
};

#endif