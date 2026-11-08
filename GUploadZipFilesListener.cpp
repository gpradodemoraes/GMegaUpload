#include "GUploadZipFilesListener.hpp"
#include "megaapi.h"
#include <string>
#include <system_error>
#include <filesystem>
#include <mutex>
#include <spdlog/spdlog.h>

namespace {
class CreateFolderListener : public mega::MegaRequestListener {
   public:
	mega::MegaNode *root = nullptr;

	void onRequestStart([[maybe_unused]] mega::MegaApi *api, [[maybe_unused]] mega::MegaRequest *request) override {
		SPDLOG_INFO("***** Folder create start");
	}

	void onRequestFinish(mega::MegaApi *api, [[maybe_unused]] mega::MegaRequest *request, mega::MegaError *e) override {
		if (root) {
			delete root;
			root = nullptr;
		}
		if (e->getErrorCode() != mega::MegaError::API_OK) {
			SPDLOG_INFO("***** Error creating folder: {}", e->getErrorString());
			return;
		}
		SPDLOG_INFO("***** Folder created successfully");
		api->fetchNodes();
	}

	void onRequestUpdate([[maybe_unused]] mega::MegaApi *api, [[maybe_unused]] mega::MegaRequest *request) override {
		SPDLOG_INFO("***** Folder create update");
	}
};

CreateFolderListener create_folder_listener;
} // namespace

void GUploadZipFilesListener::onRequestFinish(mega::MegaApi *api, mega::MegaRequest *request, mega::MegaError *e) {
	if (e->getErrorCode() != mega::MegaError::API_OK) {
		finished = true;
		return;
	}

	switch (request->getType()) {
		case mega::MegaRequest::TYPE_LOGIN: {
			api->fetchNodes();
			break;
		}
		case mega::MegaRequest::TYPE_FETCH_NODES: {
			SPDLOG_INFO("***** Showing files/folders in the root folder:");
			remote_folder_node = api->getRootNode();
			int remote_folder_found{ 0 };
			int counter{ 0 };
			for (const auto &directory : *remote_folders) {
				if (counter > remote_folder_found) break;
				counter++;

				mega::MegaNodeList *list = api->getChildren(remote_folder_node);
				for (int i = 0; i < list->size(); i++) {
					mega::MegaNode *node = list->get(i);
					if (node->isFile()) continue;
					if (node->getName() == directory) {
						SPDLOG_INFO("*****   WE FOUND HIM! : {}", node->getName());
						delete remote_folder_node;
						remote_folder_node = node->copy();
						remote_folder_found++;
						delete list;
						break;
					}
				}
			}

			if (remote_folder_found < (*remote_folders).size()) {
				// remote folder not found, create it
				create_folder_listener.root = remote_folder_node;
				api->createFolder((*remote_folders)[remote_folder_found].c_str(), remote_folder_node,
								  &create_folder_listener);
				break;
			}

			if (zip_files_directory == nullptr) {
				SPDLOG_ERROR("Directory pointer is null");
				finished = true;
				break;
			}

			std::error_code ec;
			if (!std::filesystem::is_directory(*zip_files_directory, ec)) {
				if (ec) {
					SPDLOG_ERROR("Error checking directory: {} ({})", (*zip_files_directory).string(), ec.message());
				} else {
					SPDLOG_ERROR("Error checking directory: {}", (*zip_files_directory).string());
				}
				finished = true;
				break;
			}

			const std::lock_guard<std::mutex> lock(counter_mutex);
			for (const std::filesystem::directory_entry &entry :
				 std::filesystem::directory_iterator(*zip_files_directory, ec)) {
				if (entry.is_regular_file()) {
					SPDLOG_INFO("***** Uploading {}", entry.path().filename().string());
					mega::MegaUploadOptions uploadOptions;
					uploadOptions.mtime = 0;
					api->startUpload(entry.path().string(), remote_folder_node, nullptr, &uploadOptions, nullptr);
					thread_counter++;
				}
			}
			break;
		}
		default: break;
	}
}

// Currently, this callback is only valid for the request fetchNodes()
void GUploadZipFilesListener::onRequestUpdate(mega::MegaApi *, mega::MegaRequest *request) {
	SPDLOG_INFO("***** Loading filesystem {}: {} / {}", request->getRequestString(), request->getTransferredBytes(),
				request->getTotalBytes());
}

void GUploadZipFilesListener::onRequestTemporaryError(mega::MegaApi *, mega::MegaRequest *, mega::MegaError *error) {
	SPDLOG_INFO("***** Temporary error in request: {}", error->getErrorString());
}

void GUploadZipFilesListener::onTransferFinish(mega::MegaApi *, mega::MegaTransfer *mtransf, mega::MegaError *error) {
	if (error->getErrorCode()) {
		SPDLOG_INFO("***** Transfer finished with error: {}", error->getErrorString());
	} else {
		// SPDLOG_INFO("***** Transfer finished OK: " << mtransf->getPath() << ":" << mtransf->getFileName() <<
		// std::endl;
	}
	// Lets delete the local file:
	std::error_code ec;
	bool removed = std::filesystem::remove(mtransf->getPath(), ec);
	if (ec) {
		SPDLOG_ERROR("Delete failed: {}", ec.message());
	} else if (removed) {
		SPDLOG_INFO("File {} deleted successfully", mtransf->getPath());
	} else {
		SPDLOG_WARN("Delete failed: file {} not found", mtransf->getPath());
	}
	const std::lock_guard<std::mutex> lock(counter_mutex);
	thread_counter--;
	if (thread_counter == 0) {
		finished = true;
		if (remote_folder_node) delete remote_folder_node;
	}
}

void GUploadZipFilesListener::onTransferUpdate(mega::MegaApi *, mega::MegaTransfer *transfer) {
	SPDLOG_INFO("***** Transfer progress: {} / {}", transfer->getTransferredBytes(), transfer->getTotalBytes());
}

void GUploadZipFilesListener::onTransferTemporaryError(mega::MegaApi *, mega::MegaTransfer *, mega::MegaError *error) {
	SPDLOG_INFO("***** Temporary error in transfer: {}", error->getErrorString());
}

void GUploadZipFilesListener::onUsersUpdate(mega::MegaApi *, mega::MegaUserList *users) {
	if (users == NULL) {
		// Full account reload
		return;
	}
	SPDLOG_INFO("***** There are {} new or updated users in your account", users->size());
}

void GUploadZipFilesListener::onNodesUpdate(mega::MegaApi *, mega::MegaNodeList *nodes) {
	if (nodes == NULL) {
		// Full account reload
		return;
	}

	SPDLOG_INFO("***** There are {} new or updated node/s in your account", nodes->size());
}

void GUploadZipFilesListener::onSetsUpdate(mega::MegaApi *, mega::MegaSetList *sets) {
	if (sets) {
		SPDLOG_INFO("***** There are {} new or updated Set/s in your account", sets->size());
	}
}

void GUploadZipFilesListener::onSetElementsUpdate(mega::MegaApi *, mega::MegaSetElementList *elements) {
	if (elements) {
		SPDLOG_INFO("***** There are {} new or updated Set-Element/s in your account", elements->size());
	}
}

GUploadZipFilesListener::~GUploadZipFilesListener() {
	SPDLOG_INFO("DESTRUINDO!!!!!!!");
	std::error_code errorCode;
	if (!std::filesystem::remove(*zip_files_directory, errorCode)) {
		SPDLOG_ERROR("Erro deletando diretório no destruidor: {} {}", __FUNCTION__, errorCode.message());
	}
}
