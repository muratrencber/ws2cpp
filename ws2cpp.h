#ifndef WS2_CPP
#define WS2_CPP

#define SOCK_UDP 0
#define SOCK_TCP 1

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <iostream>
#include <stdint.h>

std::ostream& operator<<(std::ostream& os, const IN_ADDR& sin_addr) {
	os << (int)sin_addr.S_un.S_un_b.s_b1 << ".";
	os << (int)sin_addr.S_un.S_un_b.s_b2 << ".";
	os << (int)sin_addr.S_un.S_un_b.s_b3 << ".";
	os << (int)sin_addr.S_un.S_un_b.s_b4;
	return os;
}

class SocketMessage {
public:
	uint16_t code;
	char* message;
	unsigned int length;

	SocketMessage(uint16_t code, const char* message, unsigned int messageLength);
	SocketMessage(char* buffer, int buflen);
	int GetSize() const;
	char* CreateBuffer() const;
	SocketMessage& operator=(const SocketMessage&);
	~SocketMessage();
};

class SocketAddress {
public:
	SocketAddress();
	SocketAddress(const char* ipv4, uint16_t port);
	SocketAddress(const sockaddr& address);
	void PrintIP() const;
	void SetAddrPort(uint16_t port);
	void SetAddrForAny();
	inline sockaddr GetAddress() const { return address; };
	inline const sockaddr* GetAddressPtr() const { return &address; };
	inline int GetSize() const { return sizeof(address); };
	int GetAddrPort() const;
	IN_ADDR GetIP() const;
private:
	sockaddr address;
};

class Socket {
public:
	int Bind(SocketAddress& address, int port, bool tryupper);
	int Bind(SocketAddress& address);
	virtual void Refresh() = 0;
	virtual int Send(const SocketMessage& msg, const SocketAddress& to) = 0;
	virtual int Receive(SocketMessage* msg, SocketAddress* from) = 0;
protected:
	SOCKET sock;
	unsigned short type;
	Socket(SOCKET sock, unsigned short type);
};

class UDPSocket : public Socket {
	friend class SockUtils;
public:
	void Refresh();
	int Send(const SocketMessage& msg, const SocketAddress& to);
	int Receive(SocketMessage* msg, SocketAddress* from);
private:
	UDPSocket(SOCKET s);
};

class TCPSocket : public Socket {

};

class AddressInfo {
public:
	friend class SockUtils;
	AddressInfo(const addrinfo& inf);
	static AddressInfo* CreateFromLL(addrinfo* addrptr);
	inline AddressInfo* GetNext() const { return this->next; };
	inline SocketAddress* GetSocketAddress()  { return &this->addr; };
	inline char* GetName() const { return inf.ai_canonname; };

private:
	addrinfo inf;
	SocketAddress addr;
	AddressInfo* next;
};

class SockUtils {
public:
	static int Initialize();
	static void Dispose();
	static AddressInfo* GetClientAddresses();
	static AddressInfo* GetDNSAddresses(const char* hostname, const char* servname);
	static SocketAddress* CreateAddressDNS(const char* hostname, const char* servname);
	static UDPSocket* CreateUDPSocket();
	static TCPSocket* CreateTCPSocket();
	static void FreeAddressInfos(AddressInfo* addr);
	static void PrintWSAD();
	static void PrintLastError();
private:
	static WSADATA* dataptr;
	SockUtils();
};

WSADATA* SockUtils::dataptr = nullptr;

SocketMessage::SocketMessage(uint16_t code, const char* message, unsigned int messageLength) :code(code), length(messageLength) {
	this->message = new char[messageLength];
	memcpy(this->message, message, messageLength);
};
SocketMessage::SocketMessage(char* buffer, int buflen) {
	char* messagePtr = buffer + sizeof(uint16_t);

	code = *buffer;
	length = buflen - sizeof(uint16_t);

	message = new char[length];
	memcpy(this->message, messagePtr, length);
};
SocketMessage& SocketMessage::operator=(const SocketMessage& msg) {
	if (message != nullptr) delete[] message;
	message = new char[msg.length];
	memcpy(this->message, msg.message, msg.length);
	this->code = msg.code;
	this->length = msg.length;
	return *this;
}
int SocketMessage::GetSize() const {
	int totalSize = sizeof(uint16_t);
	totalSize += sizeof(char) * length;
	return totalSize;
};
char* SocketMessage::CreateBuffer() const {
	char* buff = new char[GetSize()];
	uint16_t* codeptr = reinterpret_cast<uint16_t*>(buff);
	*codeptr = code;
	memcpy(buff + sizeof(uint16_t), message, length);
	return buff;
};
SocketMessage::~SocketMessage() {
	if (message != nullptr) delete[] message;
};

SocketAddress::SocketAddress() {

};
SocketAddress::SocketAddress(const char* ipv4, uint16_t port) {
	memset(&address, 0, sizeof(sockaddr));
	sockaddr_in& address_in = reinterpret_cast<sockaddr_in&>(address);
	address_in.sin_port = htons(port);
	address_in.sin_family = AF_INET;
	inet_pton(AF_INET, ipv4, &address_in.sin_addr);
};
SocketAddress::SocketAddress(const sockaddr& address) {
	this->address = address;
};
void SocketAddress::PrintIP() const {
	std::cout << GetIP() << std::endl;
};
void SocketAddress::SetAddrPort(uint16_t port) {
	sockaddr_in& address_in = reinterpret_cast<sockaddr_in&>(address);
	address_in.sin_port = htons(port);
};
void SocketAddress::SetAddrForAny() {
	sockaddr_in& address_in = reinterpret_cast<sockaddr_in&>(address);
	address_in.sin_addr.S_un.S_addr = INADDR_ANY;
};
int SocketAddress::GetAddrPort() const {
	const sockaddr_in& address_in = reinterpret_cast<const sockaddr_in&>(address);
	return ntohs(address_in.sin_port);
}
IN_ADDR SocketAddress::GetIP() const {
	const sockaddr_in& address_in = reinterpret_cast<const sockaddr_in&>(address);
	return address_in.sin_addr;
}

Socket::Socket(SOCKET sock, unsigned short type) :sock(sock), type(type) {};
int Socket::Bind(SocketAddress& address, int port, bool tryupper = false) {
	int startPort = port;
	int result;
	do {
		address.SetAddrPort(startPort);
		result = bind(sock, address.GetAddressPtr(), sizeof(sockaddr));
		startPort++;
	} while (tryupper && result != 0 && startPort > 0);
	return result;
};
int Socket::Bind(SocketAddress& address) {
	return this->Bind(address, 1, true);
};

void UDPSocket::Refresh() {
	std::cout << "Refreshing UDP Socket..." << std::endl;
};
int UDPSocket::Send(const SocketMessage& msg, const SocketAddress& to) {
	char* buffer = msg.CreateBuffer();
	int buflen = msg.GetSize();
	int result = sendto(sock, buffer, msg.GetSize(), 0, to.GetAddressPtr(), to.GetSize());
	delete[] buffer;
	return result;
};
int UDPSocket::Receive(SocketMessage* msg, SocketAddress* from) {
	char buffer[1024];
	sockaddr fromAddress; int fromLen = sizeof(sockaddr);
	int bytes = recvfrom(sock, buffer, sizeof(buffer), 0, &fromAddress, &fromLen);
	if (bytes > 0) {
		if(msg != nullptr) *msg = SocketMessage(buffer, bytes);
		if(from != nullptr) *from = SocketAddress(fromAddress);
	}
	return bytes;
};
UDPSocket::UDPSocket(SOCKET s) :Socket(s, SOCK_UDP) {};

AddressInfo::AddressInfo(const addrinfo& inf) : addr(*inf.ai_addr) {
	this->inf = inf;
	this->next = nullptr;
};
AddressInfo* AddressInfo::CreateFromLL(addrinfo* addrptr) {
	AddressInfo* lastInfo = nullptr;
	AddressInfo* firstInfo = nullptr;
	while (addrptr != nullptr) {
		AddressInfo* currentInfo = new AddressInfo(*addrptr);
		if (lastInfo != nullptr) lastInfo->next = currentInfo;
		if (firstInfo == nullptr) firstInfo = currentInfo;
		lastInfo = currentInfo;
		addrptr = addrptr->ai_next;
	}
	return firstInfo;
};

int SockUtils::Initialize() {
	dataptr = new WSADATA();
	int initResult = WSAStartup(MAKEWORD(2, 2), dataptr);
	return initResult;
};
void SockUtils::Dispose() {
	delete dataptr;
	WSACleanup();
};
AddressInfo* SockUtils::GetClientAddresses() {
	char hostname[512];
	gethostname(hostname, 512);
	return GetDNSAddresses(hostname, nullptr);
};
AddressInfo* SockUtils::GetDNSAddresses(const char* hostname, const char* servname) {
	addrinfo* result = nullptr;

	addrinfo hints;
	memset(&hints, 0, sizeof(addrinfo));
	hints.ai_family = AF_INET;

	int getRes = getaddrinfo(hostname, servname, &hints, &result);
	if (getRes != 0 || result == nullptr) return nullptr;

	AddressInfo* resultLL = AddressInfo::CreateFromLL(result);
	freeaddrinfo(result);
	return resultLL;
};
SocketAddress* SockUtils::CreateAddressDNS(const char* hostname, const char* servname) {
	AddressInfo* result = GetDNSAddresses(hostname, servname);
	if (result == nullptr) return nullptr;
	SocketAddress* newSockAddr = new SocketAddress(*result->GetSocketAddress());
	delete result;
	return newSockAddr;
};
UDPSocket* SockUtils::CreateUDPSocket() {
	SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s == INVALID_SOCKET) return nullptr;
	return new UDPSocket(s);
};
TCPSocket* SockUtils::CreateTCPSocket() {
	return nullptr;
};
void SockUtils::FreeAddressInfos(AddressInfo* addr) {
	while (addr != nullptr) {
		AddressInfo* saved = addr;
		addr = addr->next;
		delete saved;
	}
};
void SockUtils::PrintWSAD() {
	if (dataptr == nullptr) {
		std::cout << "Not initialized!" << std::endl;
		return;
	}
	std::cout << "WSA Data Information:" << std::endl;
	std::cout << "Description: " << dataptr->szDescription << std::endl;
	std::cout << "System Status: " << dataptr->szSystemStatus << std::endl;
	std::cout << "Max Sockets: " << dataptr->iMaxSockets << std::endl;
	std::cout << "Max UDP DG: " << dataptr->iMaxUdpDg << std::endl;
	std::cout << "Version: " << dataptr->wVersion << ", High Version: " << dataptr->wHighVersion << std::endl;
};
void SockUtils::PrintLastError() {
	std::cout << "Last error value: " << WSAGetLastError() << std::endl;
};
#endif // !WS2_CPP