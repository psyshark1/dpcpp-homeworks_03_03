#include <iostream>
#include <fstream>

class LogCommand {
public:
	virtual ~LogCommand() = default;
	virtual void print(const std::string& message) = 0;
};

class FileLog : public LogCommand
{
public:
	FileLog(const std::string& f_path, std::ofstream* f_str) : file_path{ f_path }, f_ostream{ f_str } {}
	void print(const std::string& message) override
	{
		f_ostream->open(file_path, std::ios::app);
		if (f_ostream->is_open())
		{
			*f_ostream << message << std::endl;
		}
	}
	~FileLog() { f_ostream->close(); }

private:
	const std::string file_path;
	std::ofstream* f_ostream;
};

class ConsoleLog : public LogCommand
{
public:
	ConsoleLog(std::ostream* p_ostr) : p_ostream{ std::move(p_ostr) } {}
	void print(const std::string& message) override
	{
		*p_ostream << message << std::endl;
	}
	~ConsoleLog() {};

private:
	std::ostream* p_ostream;
};

class LogMessage {
public:
	enum class Type
	{
		tWarning, tError, tFatalError, tUnknownMessage
	};
	virtual Type type() const = 0;
	virtual std::string message() const = 0;
	virtual ~LogMessage() {};
};

class WarningLogMessage : public LogMessage
{
public:
	WarningLogMessage(LogCommand* lc) : file{std::move(lc)} {}
	Type type() const override { return Type::tWarning; }
	std::string message() const override 
	{ 
		file->print("warning");
		return "warning";
	}
	~WarningLogMessage() {};
private:
	LogCommand* file;
};

class ErrorLogMessage : public LogMessage
{
public:
	ErrorLogMessage(LogCommand* lc) : cons{ std::move(lc) } {}
	Type type() const override { return Type::tError; }
	std::string message() const override 
	{ 
		cons->print("Error");
		return "Error"; 
	}
	~ErrorLogMessage() {};
private:
	LogCommand* cons;
};

class FatalErrorLogMessage : public LogMessage
{
public:
	FatalErrorLogMessage() {}
	Type type() const override { return Type::tFatalError; }
	std::string message() const override { throw std::runtime_error("FatalError!"); }
	~FatalErrorLogMessage() {};
};

class UnknownMessageLogMessage : public LogMessage
{
public:
	UnknownMessageLogMessage() {}
	Type type() const override { return Type::tUnknownMessage; }
	std::string message() const override { throw std::runtime_error("UnknownMessage!"); }
	~UnknownMessageLogMessage() {};
};

class LogHandler
{
public:
	LogHandler(std::unique_ptr<LogHandler> nxt) : next{ std::move(nxt) } {};
	virtual ~LogHandler() {};
	void receiveLog(const LogMessage& logmsg)
	{
		if (handleLog(logmsg)) {
			return;
		}
		if (!next) {
			throw std::runtime_error("Error: Log message should be handled!");
		}
		next->receiveLog(logmsg);
	}
protected:
	virtual bool handleLog(const LogMessage& logmsg) = 0;
	std::unique_ptr<LogHandler> next;
};

class WarningLogHandler : public LogHandler
{
public:
	WarningLogHandler(std::unique_ptr<LogHandler> nxt) : LogHandler(std::move(nxt)) {};

private:
	bool handleLog(const LogMessage& logmsg) override
	{
		if (logmsg.type() != LogMessage::Type::tWarning) { return false; }
		logmsg.message(); return true;
	}
};

class ErrorLogHandler : public LogHandler
{
public:
	ErrorLogHandler(std::unique_ptr<LogHandler> nxt) : LogHandler(std::move(nxt)) {};

private:
	bool handleLog(const LogMessage& logmsg) override
	{
		if (logmsg.type() != LogMessage::Type::tError) { return false; }
		logmsg.message(); return true;
	}
};

class FatalErrorLogHandler : public LogHandler
{
public:
	FatalErrorLogHandler(std::unique_ptr<LogHandler> nxt) : LogHandler(std::move(nxt)) {};

private:
	bool handleLog(const LogMessage& logmsg) override
	{
		if (logmsg.type() != LogMessage::Type::tFatalError) { return false; }
		logmsg.message(); return true;
	}
};

class UnknownMessageLogHandler : public LogHandler
{
public:
	UnknownMessageLogHandler(std::unique_ptr<LogHandler> nxt) : LogHandler(std::move(nxt)) {};

private:
	bool handleLog(const LogMessage& logmsg) override
	{
		if (logmsg.type() != LogMessage::Type::tUnknownMessage) { return false; }
		logmsg.message(); return true;
	}
};



int main()
{
	ConsoleLog cl(&std::cout);
	std::ofstream ofs;
	FileLog fl("file_path", &ofs);

	auto warn_handler = std::make_unique<WarningLogHandler>(nullptr);
	auto err_handler = std::make_unique<ErrorLogHandler>(std::move(warn_handler));
	auto fatalerr_handler = std::make_unique<FatalErrorLogHandler>(std::move(err_handler));
	auto unknowmes_handler = std::make_unique<UnknownMessageLogHandler>(std::move(fatalerr_handler));
	
	try
	{
		unknowmes_handler->receiveLog(WarningLogMessage(&cl));
		unknowmes_handler->receiveLog(ErrorLogMessage(&fl));
		unknowmes_handler->receiveLog(UnknownMessageLogMessage());
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	return 0;
}
