#include "megaapi.h"
#include <mutex>

class FastLoginListener : public mega::MegaRequestListener {
   public:
	bool isLoginSuccessful() const { return login_successful; }

	void onRequestStart([[maybe_unused]] mega::MegaApi *api, [[maybe_unused]] mega::MegaRequest *request) override {
		SPDLOG_INFO("***** Fast login start");
	}

	void onRequestFinish([[maybe_unused]] mega::MegaApi *api, [[maybe_unused]] mega::MegaRequest *request,
						 mega::MegaError *e) override {
		SPDLOG_INFO("***** Fast login finished successfully");
		if (request->getType() == mega::MegaRequest::TYPE_LOGIN) {
			std::lock_guard<std::mutex> lock(fastlogin_mtx);
			if (e->getErrorCode() == mega::MegaError::API_OK) {
				// fast login worked!
				SPDLOG_INFO("fastLogin successful!");
				login_successful = true;
				fastloginfinshed = true;
			} else {
				// Session invalid/expired -> fall back to email/password login
				SPDLOG_ERROR("fastLogin failed: {}", e->getErrorString());
				fastloginfinshed = true;
			}
			fastlogin_cv.notify_one();
		}
	}

	void onRequestUpdate([[maybe_unused]] mega::MegaApi *api, [[maybe_unused]] mega::MegaRequest *request) override {
		SPDLOG_INFO("***** Fast login update");
	}

   private:
	bool login_successful = false;
};
