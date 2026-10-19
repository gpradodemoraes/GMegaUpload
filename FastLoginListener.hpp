class FastLoginListener : public mega::MegaRequestListener {
   public:
	bool isLoginSuccessful() const { return login_successful; }

	void onRequestStart([[maybe_unused]] mega::MegaApi *api, [[maybe_unused]] mega::MegaRequest *request) override {
		std::cout << "***** Fast login start" << std::endl;
	}

	void onRequestFinish([[maybe_unused]] mega::MegaApi *api, [[maybe_unused]] mega::MegaRequest *request,
						 mega::MegaError *e) override {
		std::cout << "***** Fast login finished successfully" << std::endl;
		if (request->getType() == mega::MegaRequest::TYPE_LOGIN) {
			std::lock_guard<std::mutex> lock(fastlogin_mtx);
			if (e->getErrorCode() == mega::MegaError::API_OK) {
				// fast login worked!
				std::cout << "fastLogin successful!" << std::endl;
				login_successful = true;
				fastloginfinshed = true;
			} else {
				// Session invalid/expired -> fall back to email/password login
				std::cerr << "fastLogin failed: " << e->getErrorString() << std::endl;
				fastloginfinshed = true;
			}
			fastlogin_cv.notify_one();
		}
	}

	void onRequestUpdate([[maybe_unused]] mega::MegaApi *api, [[maybe_unused]] mega::MegaRequest *request) override {
		std::cout << "***** Fast login update" << std::endl;
	}

   private:
	bool login_successful = false;
};
