#pragma once

#include <mysql.h>
#include <ctime>
#include <string>
using namespace std;

/*
实现MySQL数据库的增删改查操作
*/
class Connection
{
public:
	// 初始化数据库连接
	Connection();

	// 释放数据库连接资源
	~Connection();

	// 连接数据库
	bool connect(string ip, 
				 unsigned short port, 
				 string user, 
				 string password, 
				 string dbname);

	// 更新操作 insert、delete、update
	bool update(string sql);

	// 查询操作 select
	MYSQL_RES* query(string sql);

	// 刷新连接的闲置时间起始点
	void refreshAliveTime();

	// 返回闲置时间
	clock_t getAliveTime() const;

private:
	MYSQL* _conn;	// 表示和MySQL Server的一条连接
	clock_t _aliveTime;	// 记录闲置时间起始点
};