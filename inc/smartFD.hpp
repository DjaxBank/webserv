
#ifndef SMARTFD_HPP
#define SMARTFD_HPP

#include <unistd.h>

class smartFD 
{
	private:
		int m_fd;
	public:
		smartFD() noexcept;
		explicit smartFD(int fd) noexcept;
		smartFD(const smartFD &other) = delete;
		smartFD& operator=(const smartFD &other) = delete;
		smartFD(smartFD &&other) noexcept;
		smartFD & operator=(smartFD &&other) noexcept;
		~smartFD() noexcept;
		const int& getFd() const;

};

#endif