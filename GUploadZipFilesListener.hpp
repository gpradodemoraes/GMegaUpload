#ifndef _GUPLOADZIPFILESLISTENER_H_
#define _GUPLOADZIPFILESLISTENER_H_
#include <megaapi.h>
#include <mutex>
#include <string>
#include <filesystem>
#include "GListener.hpp"
#include <memory>
#include <utility>
#include <vector>

class GUploadZipFilesListener : public GListener {
   public:
	GUploadZipFilesListener(std::unique_ptr<std::filesystem::path> path, const std::vector<std::string> *remote_folders)
		: zip_files_directory(std::move(path)), remote_folders(remote_folders) {};

   private:
	std::unique_ptr<std::filesystem::path> zip_files_directory;
	const std::vector<std::string> *remote_folders;
	mega::MegaNode *remote_folder_node = nullptr;
	std::mutex counter_mutex;
	int thread_counter = 0;

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
	~GUploadZipFilesListener();
};
#endif
