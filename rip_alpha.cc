/*
 * rip_alpha.cc
 *
 *  Created on: Oct 28, 2021
 *      Author: bach
 */

#include "task.h"

/*
 std::random_device rd; // 真实随机数产生器

 std::mt19937 mt(rd()); //生成计算随机数mt

 std::uniform_int_distribution<int> dist(-1000, 1000); //生成-1000到1000之间的离散均匀分布数

 auto rnd = std::bind(dist, mt);

 // 设置线程睡眠时间
 void simulate_hard_computation()
 {
 std::this_thread::sleep_for(std::chrono::milliseconds(2000 + rnd()));
 }

 // 添加两个数字的简单函数并打印结果
 void multiply(const int a, const int b)
 {
 simulate_hard_computation();
 const int res = a * b;
 std::cout << a << " * " << b << " = " << res << std::endl;
 }

 // 添加并输出结果
 void multiply_output(int &out, const int a, const int b)
 {
 simulate_hard_computation();
 out = a * b;
 std::cout << a << " * " << b << " = " << out << std::endl;
 }

 // 结果返回
 int multiply_return(const int a, const int b)
 {
 simulate_hard_computation();
 const int res = a * b;
 std::cout << a << " * " << b << " = " << res << std::endl;
 return res;
 }
 */

void submit_task() {

#if (USED_VER_1)

#elif (USED_VER_2)


#elif (USED_VER_3)
#elif (USED_VER_4)
#elif (USED_VER_5)
#elif (USED_VER_6)
#elif (USED_VER_7)
#elif (USED_VER_8)
    thread_pool pool(8);
    pool.init();

/*
    std::vector<std::tuple<std::string, std::string>> result;
    for (int y = 2005; y <= 2021; ++y) {
    	for (int m = 1; m <= 12; ++m) {
    		for (int d = 1; d <= 31; ++d) {
    			char url[512] = {};
    			sprintf(url, "https://www.alphadictionary.com/goodword/date/%d/%d/%d", y, m, d);
    			auto future = pool.submit(fetch_url, url);
    			future.get();
    		}
    	}
    }
*/
    auto future = pool.submit(fetch_url, "https://www.alphadictionary.com/goodword/date/2021/10/23");
    future.get();

    pool.shutdown();
#else


#endif

	/*
	 int output_ref;
	 auto future1 = pool.submit(multiply_output, std::ref(output_ref), 5, 6);

	 future1.get();
	 std::cout << "Last operation result is equals to " << output_ref << std::endl;

	 auto future2 = pool.submit(multiply_return, 5, 3);

	 int res = future2.get();
	 std::cout << "Last operation result is equals to " << res << std::endl;
	 */

}

int main() {

	fixed_thread_pool pool(8);

	for (int y = 2005; y <= 2021; ++y) {
		for (int m = 1; m <= 12; ++m) {
			for (int d = 1; d <= 31; ++d) {
				char url[512] = { };
				sprintf(url,
						"https://www.alphadictionary.com/goodword/date/%d/%d/%d",
						y, m, d);
				//auto future = pool.submit(fetch_url, url);
				//future.get();
				std::cout << url << '\n';
				pool.execute([=]() {
					fetch_html(url);
				});
			}
		}
	}

	while (true) {
	}
}
