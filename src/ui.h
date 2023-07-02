#include <string>
#include <functional>
#include <memory>
#include <condition_variable>
#include <atomic>

namespace lpvpn::ui {
	enum EventCode {
		EXIT = 1,
	};

	class Event {
		public:
		Event(EventCode code) : code(code) {};
		EventCode code;
	};

	struct MenuItem {
		std::string text;
		std::shared_ptr<std::vector<MenuItem>> submenu = nullptr;
		std::function<void(MenuItem&)> cb = nullptr;
		std::shared_ptr<void> userData = nullptr;
	};

	class UI {
		public:
		UI();
		~UI();

		void notify(const std::string &title, const std::string &text);
		void alert(const std::string &title, const std::string &text);
		void onEvent(std::function<void(Event&)> cb);
		void setMenu(std::unique_ptr<MenuItem> menu);
		void setClipboard(const std::string &text);
		void openURL(const std::string &url);

		private:
		// pimpl
		class Impl;
		std::unique_ptr<Impl> impl;
	};
}
