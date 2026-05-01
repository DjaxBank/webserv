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

int smartFD::getFd() const
{
	return this->m_fd;
}

/* For exceptional cases when we need to handle releasing manually
	ie. if we use exceve, it object would destroy itelf before it ran
	so you store it in a var and hand it off */
int smartFD::release() noexcept
{
	int fd = m_fd;
	m_fd = -1;
	return fd;
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