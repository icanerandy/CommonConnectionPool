#pragma once
#include <iostream>
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>
#include "Connection.h"
#include "public.h"
/*
ʵ�����ӳع���ģ��
*/
class ConnectionPool
{
public:
	// ��ȡ���ӳض���ʵ��
	static ConnectionPool* getConnectionPool();

	// ���ⲿ�ṩ�ӿڣ������ӳ��л�ȡһ�����õĿ�������
	std::shared_ptr<Connection> getConnection();

	// �����ڶ����߳��У�ר�Ÿ��������µ�����
	void produceConnectionTask();

private:
	// ����#1 ���캯��˽�л�
	ConnectionPool();

	// �������ļ��м���������
	bool loadConfigFile();

	// ɨ�賬��maxIdleTimeʱ��Ŀ������ӣ����жԶ������ӵĻ���
	void scannerConnectionTask();

private:
	std::string _ip;			// mysql��ip��ַ
	unsigned short _port;		// mysql�Ķ˿ں� 3306
	std::string _username;		// mysql��¼�û���
	std::string _password;		// mysql��¼����
	std::string _dbname;		// ���ӵ����ݿ�����
	size_t _initSize;			// ���ӳصĳ�ʼ������
	size_t _maxSize;			// ���ӳص����������
	size_t _maxIdleTime;		// ���ӳ�������ʱ��
	size_t _connectionTimeout;	// ���ӳػ�ȡ���ӵĳ�ʱʱ��

	std::queue<Connection*> _connectionQueue;	// �洢mysql���ӵĶ���
	std::mutex _queueMutex;		// ά��mysql���Ӷ��е��̰߳�ȫ������

	atomic_int _connectionCnt;	// ��¼������������connection��������
	std::condition_variable _cv;// ���������������������������ߺ��������̵߳�ͨ��
};