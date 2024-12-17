#pragma once

#include <mysql.h>
#include <ctime>
#include <string>
using namespace std;

/*
ʵ��MySQL���ݿ����ɾ�Ĳ����
*/
class Connection
{
public:
	// ��ʼ�����ݿ�����
	Connection();

	// �ͷ����ݿ�������Դ
	~Connection();

	// �������ݿ�
	bool connect(string ip, 
				 unsigned short port, 
				 string user, 
				 string password, 
				 string dbname);

	// ���²��� insert��delete��update
	bool update(string sql);

	// ��ѯ���� select
	MYSQL_RES* query(string sql);

	// ˢ�����ӵĿ���ʱ����ʼ��
	void refreshAliveTime()
	{
		_aliveTime = clock();
	}

	// ���ؿ���ʱ��
	clock_t getAliveTime() const
	{
		return clock() - _aliveTime;
	}

private:
	MYSQL* _conn;	// ��ʾ��MySQL Server��һ������
	clock_t _aliveTime;	// ��¼����ʱ����ʼ��
};