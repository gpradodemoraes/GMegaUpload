#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <megaapi.h>
#include <mutex>
#include <string>
#include <thread>
#include <time.h>
#include <filesystem>

class GListener : public mega::MegaListener {
   private:
	const std::filesystem::path *zip_files_directory;
	const std::string remote_folder;
	mega::MegaNode *remote_folder_node = nullptr;

   public:
	bool finished{};

	GListener(std::filesystem::path *path, std::string remote_folder)
		: zip_files_directory(path), remote_folder(remote_folder) {};

	virtual void onRequestFinish(mega::MegaApi *api, mega::MegaRequest *request, mega::MegaError *e);

	virtual void onRequestUpdate(mega::MegaApi *, mega::MegaRequest *request);

	virtual void onRequestTemporaryError(mega::MegaApi *, mega::MegaRequest *, mega::MegaError *error);

	virtual void onTransferFinish(mega::MegaApi *, mega::MegaTransfer *, mega::MegaError *error);

	virtual void onTransferUpdate(mega::MegaApi *, mega::MegaTransfer *transfer);

	virtual void onTransferTemporaryError(mega::MegaApi *, mega::MegaTransfer *, mega::MegaError *error);

	virtual void onUsersUpdate(mega::MegaApi *, mega::MegaUserList *users);

	virtual void onNodesUpdate(mega::MegaApi *, mega::MegaNodeList *nodes);

	virtual void onSetsUpdate(mega::MegaApi *, mega::MegaSetList *sets);

	virtual void onSetElementsUpdate(mega::MegaApi *, mega::MegaSetElementList *elements);
};
