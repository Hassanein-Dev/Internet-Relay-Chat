#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <arpa/inet.h>
#include <fstream>
#include <sys/stat.h>
#include <csignal>

#define MAX_CLIENT 100
#define BUFFER_SIZE 512

class Channel
{
private:
	std::string name;
	std::vector<int> clients;
	std::vector<int> admins;
	std::vector<int> invited;
	std::string topic;

	bool invite_only;
	bool topic_locked;
	std::string password;
	int user_limit;

public:
	Channel() : invite_only(false), topic_locked(false), user_limit(0) {}

	const std::string &getName() const { return name; }
	const std::vector<int> &getClients() const { return clients; }
	const std::vector<int> &getAdmins() const { return admins; }
	const std::vector<int> &getInvited() const { return invited; }
	const std::string &getTopic() const { return topic; }

	bool isInviteOnly() const { return invite_only; }
	bool isTopicLocked() const { return topic_locked; }
	const std::string &getPassword() const { return password; }
	int getUserLimit() const { return user_limit; }

	void setName(const std::string &Name) { name = Name; }
	void setTopic(const std::string &Topic) { topic = Topic; }
	void setInviteOnly(bool value) { invite_only = value; }
	void setTopicLocked(bool value) { topic_locked = value; }
	void setPassword(const std::string &Password) { password = Password; }
	void setUserLimit(int value) { user_limit = value; }

	void addClient(int clientfd) { clients.push_back(clientfd); }
	void removeClient(int clientfd) { clients.erase(std::remove(clients.begin(), clients.end(), clientfd), clients.end()); }
	bool hasClient(int clientfd) const { return std::find(clients.begin(), clients.end(), clientfd) != clients.end(); }

	void addAdmin(int clientfd) { admins.push_back(clientfd); }
	void removeAdmin(int clientfd) { admins.erase(std::remove(admins.begin(), admins.end(), clientfd), admins.end()); }
	bool isAdmin(int clientfd) const { return std::find(admins.begin(), admins.end(), clientfd) != admins.end(); }

	void addInvited(int clientfd) { invited.push_back(clientfd); }
	void removeInvited(int clientfd) { invited.erase(std::remove(invited.begin(), invited.end(), clientfd), invited.end()); }
	bool isInvited(int clientfd) const { return std::find(invited.begin(), invited.end(), clientfd) != invited.end(); }
};

class Client
{
private:
	bool authenticated;
	bool pass_given;
	bool nick_given;
	bool user_given;

	std::string nickname;
	std::string username;
	std::string realname;
	std::string hostname;

	std::string buffer;

public:
	Client() : authenticated(false), pass_given(false), nick_given(false), user_given(false) {}

	bool isAuthenticated() const { return authenticated; }
	bool isPassGiven() const { return pass_given; }
	bool isNickGiven() const { return nick_given; }
	bool isUserGiven() const { return user_given; }

	void setAuthenticated(bool value) { authenticated = value; }
	void setPassGiven(bool value) { pass_given = value; }
	void setNickGiven(bool value) { nick_given = value; }
	void setUserGiven(bool value) { user_given = value; }

	const std::string &getNickname() const { return nickname; }
	const std::string &getUsername() const { return username; }
	const std::string &getRealname() const { return realname; }
	const std::string &getHostname() const { return hostname; }
	const std::string &getBuffer() const { return buffer; }

	void setNickname(const std::string &nick) { nickname = nick; }
	void setUsername(const std::string &user) { username = user; }
	void setRealname(const std::string &real) { realname = real; }
	void setHostname(const std::string &host) { hostname = host; }

	void appendBuffer(const std::string &data) { buffer += data; }
	void clearBuffer() { buffer.clear(); }
	void eraseBuffer(size_t pos, size_t len) { buffer.erase(pos, len); }
	std::string &getBufferRef() { return buffer; }
};

class ServerEnv
{
private:
	int port;
	std::string password;
	int server_fd;

	std::map<int, Client> clients;
	std::map<std::string, Channel> channels;

public:
	ServerEnv() : port(0), server_fd(-1) {}

	// getters
	int getPort() const { return port; }
	const std::string &getPass() const { return password; }
	int getServerFd() const { return server_fd; }

	// setters
	void setPort(int value) { port = value; }
	void setPassword(const std::string &value) { password = value; }
	void setServerFd(int value) { server_fd = value; }

	// client
	Client &getClient(int fd) { return clients[fd]; }
	const Client &getClient(int fd) const
	{
		std::map<int, Client>::const_iterator it = clients.find(fd);
		if (it == clients.end())
		{
			throw std::runtime_error("Client not found");
		}
		return it->second;
	}
	bool hasClient(int fd) const { return clients.find(fd) != clients.end(); }
	void addClient(int fd, const Client &client) { clients[fd] = client; }
	void removeClient(int fd) { clients.erase(fd); }
	std::map<int, Client> &getClients() { return clients; }
	const std::map<int, Client> &getClients() const { return clients; }

	// channel
	Channel &getChannel(const std::string &name) { return channels[name]; }
	const Channel &getChannel(const std::string &name) const
	{
		std::map<std::string, Channel>::const_iterator it = channels.find(name);
		if (it == channels.end())
		{
			throw std::runtime_error("Channel not found");
		}
		return it->second;
	}
	bool hasChannel(const std::string &name) const { return channels.find(name) != channels.end(); }
	void addChannel(const std::string &name, const Channel &channel) { channels[name] = channel; }
	void removeChannel(const std::string &name) { channels.erase(name); }
	std::map<std::string, Channel> &getChannels() { return channels; }
	const std::map<std::string, Channel> &getChannels() const { return channels; }
};

// Functions

// Serv
int create_server_socket(int port);
void set_non_blocking(int fd);

// Main loop
void server_loop(ServerEnv *env);
void cleanup_server(ServerEnv *env);

// Client Handling
void handle_client_data(int client_fd, ServerEnv *env);
void disconnect_client(int client_fd, ServerEnv *env);

// Command handlers
void handle_pass(int client_fd, const std::string &cmd, ServerEnv *env);
void handle_nick(int client_fd, std::string &cmd, ServerEnv *env);
void handle_user(int client_fd, const std::string &cmd, ServerEnv *env);
void handle_join(int client_fd, const std::string &cmd, ServerEnv *env);
void handle_privmsg(int client_fd, const std::string &cmd, ServerEnv *env);
bool handle_bot_command(int client_fd, const std::string &channel_name, const std::string &message, ServerEnv *env);
void handle_kick(int client_fd, const std::string &cmd, ServerEnv *env);
void handle_invite(int client_fd, const std::string &cmd, ServerEnv *env);
void handle_topic(int client_fd, const std::string &cmd, ServerEnv *env);
void handle_mode(int client_fd, const std::string &cmd, ServerEnv *env);
void handle_who(int client_fd, const std::string &cmd, ServerEnv *env);
void handle_whois(int client_fd, const std::string &cmd, ServerEnv *env);
void handle_sendfile(int client_fd, const std::string &cmd, ServerEnv *env);
void handle_getfile(int client_fd, const std::string &cmd, ServerEnv *env);
std::string int_to_string(int value);

// Utility functions
void send_message(int fd, const std::string &msg);
std::string trim(const std::string &str);
std::vector<std::string> split_string(const std::string &str, char delimiter);
bool is_client_in_channel(int client_fd, const std::string &channel_name, ServerEnv *env);
void broadcast_to_channel(const std::string &msg, const std::string &channel_name, ServerEnv *env, int exclude_fd = -1);
bool is_client_operator(int client_fd, const std::string &channel_name, ServerEnv *env);

#endif