#include "Connection.h"
#include "ConnectionPool.h"

#include <iostream>

int main()
{
	clock_t begin = clock();

#if 0
	for (size_t i = 0; i < 1000; ++i)
	{
		char sql[1024] = { 0 };
		sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
				"zhang san", 20, "male");

		Connection conn;
		conn.connect("127.0.0.1", 3306, "root", "513921", "chat");
		conn.update(sql);

		//ConnectionPool* cp = ConnectionPool::getConnectionPool();
		//std::shared_ptr<Connection> sp = cp->getConnection();
		//sp->update(sql);
	}
#endif

#if 1
	// 多线程环境下，如果不适用MySQL连接池，而是每个线程在短时间内频繁地创建和销毁连接，可能会导致MySQL查询失败。
	// 而使用MySQL连接池，可以：1.限制连接数量；2.复用连接；3.管理连接生命周期；4.减少延迟。
	std::thread th1([] () {
		for (size_t i = 0; i < 2500; ++i)
		{
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
					"zhang san", 20, "male");

			//Connection conn;
			//conn.connect("127.0.0.1", 3306, "root", "513921", "chat");
			//conn.update(sql);

			ConnectionPool* cp = ConnectionPool::getConnectionPool();
			std::shared_ptr<Connection> sp = cp->getConnection();
			sp->update(sql);
		}
	});

	std::thread th2([] () {
		for (size_t i = 0; i < 2500; ++i)
		{
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
					"zhang san", 20, "male");

			//Connection conn;
			//conn.connect("127.0.0.1", 3306, "root", "513921", "chat");
			//conn.update(sql);

			ConnectionPool* cp = ConnectionPool::getConnectionPool();
			std::shared_ptr<Connection> sp = cp->getConnection();
			sp->update(sql);
		}
	});

	std::thread th3([] () {
		for (size_t i = 0; i < 2500; ++i)
		{
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
					"zhang san", 20, "male");

			//Connection conn;
			//conn.connect("127.0.0.1", 3306, "root", "513921", "chat");
			//conn.update(sql);

			ConnectionPool* cp = ConnectionPool::getConnectionPool();
			std::shared_ptr<Connection> sp = cp->getConnection();
			sp->update(sql);
		}
	});

	std::thread th4([] () {
		for (size_t i = 0; i < 2500; ++i)
		{
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')",
					"zhang san", 20, "male");

			//Connection conn;
			//conn.connect("127.0.0.1", 3306, "root", "513921", "chat");
			//conn.update(sql);

			ConnectionPool* cp = ConnectionPool::getConnectionPool();
			std::shared_ptr<Connection> sp = cp->getConnection();
			sp->update(sql);
		}
	});

	th1.join();
	th2.join();
	th3.join();
	th4.join();
#endif

	clock_t end = clock();
	std::cout << (end - begin) << "ms" << std::endl;

	return 0;
}