#include "smartFD.hpp"

smartFD::smartFD() noexcept : m_fd(-1)
{
};

smartFD::smartFD(int fd) noexcept : m_fd(fd)
{
};


smartFD::~smartFD() noexcept
{
	if (m_fd >= 0)
		close(m_fd);
}

const int& smartFD::getFd() const
{
	return (this->m_fd);
}
smartFD::smartFD(smartFD &&other) noexcept : m_fd(other.m_fd)
{
	other.m_fd = -1;
}

smartFD& smartFD::operator=(smartFD &&other) noexcept
{
	if (this != &other)
	{
		if (this->m_fd >= 0)
		{
			close(this->m_fd);
		}
		this->m_fd = other.m_fd;
		other.m_fd = -1;
	}
	return *this;
}