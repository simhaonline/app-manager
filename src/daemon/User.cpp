#include "User.h"
#include "../common/Utility.h"


//////////////////////////////////////////////////////////////////////
/// Users
//////////////////////////////////////////////////////////////////////
Users::Users()
{
}

Users::~Users()
{
}

web::json::value Users::AsJson()
{
	std::lock_guard<std::recursive_mutex> guard(m_mutex);
	web::json::value result = web::json::value::object();
	for (auto user : m_users)
	{
		result[user.first] = user.second->AsJson();
	}
	return std::move(result);
}

std::shared_ptr<Users> Users::FromJson(const web::json::value& obj, std::shared_ptr<Roles> roles)
{
	std::shared_ptr<Users> users = std::make_shared<Users>();
	auto jsonOj = obj.as_object();
	for (auto user : jsonOj)
	{
		auto name = GET_STD_STRING(user.first);
		users->m_users[name] = User::FromJson(name, user.second, roles);
	}
	return users;
}

std::map<std::string, std::shared_ptr<User>> Users::getUsers()
{
	std::lock_guard<std::recursive_mutex> guard(m_mutex);
	return m_users;
}

std::shared_ptr<User> Users::getUser(std::string name)
{
	std::lock_guard<std::recursive_mutex> guard(m_mutex);
	auto user = m_users.find(name);
	if (user != m_users.end())
	{
		return user->second;
	}
	else
	{
		throw std::invalid_argument("no such user.");
	}
}

void Users::addUsers(const web::json::value& obj, std::shared_ptr<Roles> roles)
{
	if (!obj.is_null() && obj.is_object())
	{
		auto users = obj.as_object();
		for (auto userJson : users)
		{
			addUser(GET_STD_STRING(userJson.first), userJson.second, roles);
		}
	}
	else
	{
		throw std::invalid_argument("incorrect user json section");
	}
}

std::shared_ptr<User> Users::addUser(const std::string userName, const web::json::value& userJson, std::shared_ptr<Roles> roles)
{
	std::lock_guard<std::recursive_mutex> guard(m_mutex);
	auto user = User::FromJson(userName, userJson, roles);
	m_users[userName] = user;
	return user;
}

void Users::delUser(std::string name)
{
	std::lock_guard<std::recursive_mutex> guard(m_mutex);
	getUser(name);
	m_users.erase(name);
}

//////////////////////////////////////////////////////////////////////
/// User
//////////////////////////////////////////////////////////////////////
User::User(std::string name) :m_locked(false), m_name(name)
{
}

User::~User()
{
}

web::json::value User::AsJson()
{
	std::lock_guard<std::recursive_mutex> guard(m_mutex);
	web::json::value result = web::json::value::object();

	result[JSON_KEY_USER_key] = web::json::value::string(m_key);
	result[JSON_KEY_USER_locked] = web::json::value::boolean(m_locked);
	auto roles = web::json::value::array(m_roles.size());
	int i = 0;
	for (auto role : m_roles)
	{
		roles[i++] = web::json::value::string(role->getName());
	}
	result[JSON_KEY_USER_roles] = roles;
	return result;
}

std::shared_ptr<User> User::FromJson(std::string userName, const web::json::value& obj, const std::shared_ptr<Roles> roles)
{
	std::shared_ptr<User> result;
	if (!obj.is_null())
	{
		if (!HAS_JSON_FIELD(obj, JSON_KEY_USER_key))
		{
			throw std::invalid_argument("user should have key attribute");
		}
		result = std::make_shared<User>(userName);
		result->m_key = GET_JSON_STR_VALUE(obj, JSON_KEY_USER_key);
		result->m_locked = GET_JSON_BOOL_VALUE(obj, JSON_KEY_USER_locked);
		if (HAS_JSON_FIELD(obj, JSON_KEY_USER_roles))
		{
			auto arr = obj.at(JSON_KEY_USER_roles).as_array();
			for (auto jsonRole : arr) result->m_roles.insert(roles->getRole(jsonRole.as_string()));
		}
	}
	return result;
}

void User::lock()
{
	this->m_locked = true;
}

void User::unlock()
{
	this->m_locked = false;
}

void User::updateRoles(std::set<std::shared_ptr<Role>> roles)
{
	this->m_roles = roles;
}

void User::updateKey(std::string passswd)
{
	std::lock_guard<std::recursive_mutex> guard(m_mutex);
	m_key = passswd;
}

bool User::locked() const
{
	return m_locked;
}

const std::string User::getKey()
{
	std::lock_guard<std::recursive_mutex> guard(m_mutex);
	return m_key;
}

const std::set<std::shared_ptr<Role>> User::getRoles()
{
	std::lock_guard<std::recursive_mutex> guard(m_mutex);
	return m_roles;
}

bool User::hasPermission(std::string permission)
{
	std::lock_guard<std::recursive_mutex> guard(m_mutex);
	for (auto role : m_roles)
	{
		if (role->hasPermission(permission)) return true;
	}
	return false;
}

