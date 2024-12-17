#include "ConnectionPool.h"

// �̰߳�ȫ���������������ӿ�
ConnectionPool* ConnectionPool::getConnectionPool()
{
	static ConnectionPool pool;	// ��̬�ֲ������ĳ�ʼ�����ɱ������Զ�ʵ��lock��unlock
	return &pool;
}

// ���ⲿ�ṩ�ӿڣ������ӳ��л�ȡһ�����õĿ�������
std::shared_ptr<Connection> ConnectionPool::getConnection()
{
	std::unique_lock<std::mutex> lock(_queueMutex);
	while (_connectionQueue.empty())
	{
		if (_cv.wait_for(lock, std::chrono::milliseconds(_connectionTimeout)) == std::cv_status::timeout)
		{
			LOG("��ȡ�������ӳ�ʱ����ȡ����ʧ�ܣ�");
			return nullptr;
		}
	}

	/*
	shared_ptr����ָ������ʱ���Ὣconnection������Դֱ��delete�����൱�ڵ���connection������������connection�ͱ�close���ˡ�
	����������Ҫ�Զ���shared_ptr���ͷ���Դ��ʽ����connectionֱ�ӹ黹��queue����
	*/
	std::shared_ptr<Connection> sp(_connectionQueue.front(),
									[&] (Connection* conn) {
										std::unique_lock<std::mutex> lock(_queueMutex);	// �������ڷ�����Ӧ���߳��е��õģ�����һ��Ҫ���Ƕ��е��̰߳�ȫ����
										conn->refreshAliveTime();	// ˢ�¿���ʱ����ʼ��
										_connectionQueue.push(conn);
									});
	_connectionQueue.pop();
	if (_connectionQueue.empty())
	{
		_cv.notify_all();	// #1 ˭�����˶����е����һ��connection��˭�͸���֪ͨ��������������
	}
	//_cv.notify_all();	// #2 ���������Ӻ�֪ͨ�������̼߳��һ�£��������Ϊ���ˣ��Ͻ������µ�����
	return sp;
}

// �������ļ��м���������
bool ConnectionPool::loadConfigFile()
{
	FILE* pf = fopen("mysql.ini", "r");
	if (pf == nullptr)
	{
		LOG("mysql.ini file does not exist!");
		return false;
	}

	while (!feof(pf))
	{
		char line[1024] = { 0 };
		fgets(line, 1024, pf);
		std::string str = line;

		int idx = str.find('=');
		if (idx == -1)	// ��Ч��������/ע��
		{
			continue;
		}

		// username=root\n
		int endIdx = str.find('\n', idx);
		std::string key = str.substr(0, idx);
		std::string value = str.substr(idx + 1, endIdx - idx - 1);

		if (key == "ip")
		{
			_ip = value;
		}
		else if (key == "port")
		{
			_port = atoi(value.c_str());
		}
		else if (key == "username")
		{
			_username = value;
		}
		else if (key == "password")
		{
			_password = value;
		}
		else if (key == "dbname")
		{
			_dbname = value;
		}
		else if (key == "initSize")
		{
			_initSize = atoi(value.c_str());
		}
		else if (key == "maxSize")
		{
			_maxSize = atoi(value.c_str());
		}
		else if (key == "maxIdleTimeout")
		{
			_maxIdleTime = atoi(value.c_str());
		}
		else if (key == "connectionTimeout")
		{
			_connectionTimeout = atoi(value.c_str());
		}
	}

	return true;
}

void ConnectionPool::scannerConnectionTask()
{
	for (;;)
	{
		// ͨ��sleepģ�ⶨʱЧ��
		std::this_thread::sleep_for(std::chrono::seconds(_maxIdleTime));

		// ɨ���������У��ͷŶ��������
		std::unique_lock<std::mutex> lock(_queueMutex);
		while (_connectionCnt > _initSize)
		{
			Connection* conn = _connectionQueue.front();
			if (conn->getAliveTime() >= _maxIdleTime * 1000)
			{
				_connectionQueue.pop();
				_connectionCnt--;
				delete conn;	// ����~Connection()�ͷ�����
			}
			else
			{
				break;	// ��ͷ������û�г���_maxIdleTime���������ӿ϶�Ҳû�У������Ƕ�ͷ�Ƚ���
			}
		}
	}
}

// �����ڶ����߳��У�ר�Ÿ��������µ�����
void ConnectionPool::produceConnectionTask()
{
	for (;;)
	{
		std::unique_lock<std::mutex> lock(_queueMutex);
		while (!_connectionQueue.empty())
		{
			_cv.wait(lock);	// ���в��գ��˴������߳̽���ȴ�״̬
		}

		// ��������û�е������ޣ����������µ�����
		if (_connectionCnt < _maxSize)
		{
			Connection* conn = new Connection();
			conn->connect(_ip, _port, _username, _password, _dbname);
			conn->refreshAliveTime();	// ˢ�¿���ʱ����ʼ��
			_connectionQueue.push(conn);
			_connectionCnt++;
		}

		// ֪ͨ�������߳̿������������ˣ��������ɵȴ� => ������
		_cv.notify_all();
	}
}

// ���ӳصĹ���
ConnectionPool::ConnectionPool()
{
	// ����������
	if (!loadConfigFile())
	{
		return;
	}

	// ������ʼ����������
	for (size_t i = 0; i < _initSize; ++i)
	{
		Connection* conn = new Connection();
		conn->connect(_ip, _port, _username, _password, _dbname);
		conn->refreshAliveTime();	// ˢ�¿���ʱ����ʼ��
		_connectionQueue.push(conn);
		_connectionCnt++;
	}

	// ����һ���µ��̣߳���Ϊ���ӵ������� Linux��thread�ײ���õľ���pthread
	std::thread produceTh(std::bind(&ConnectionPool::produceConnectionTask, this));
	produceTh.detach();	// ����Ϊ�ػ��̣߳����룩
	// ����һ���µĶ�ʱ�̣߳�ɨ�賬��maxIdleTimeʱ��Ŀ������ӣ����жԶ������ӵĻ���
	std::thread scannerTh(std::bind(&ConnectionPool::scannerConnectionTask, this));
	scannerTh.detach();	// ����Ϊ�ػ��̣߳����룩
}
