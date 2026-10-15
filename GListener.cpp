#include "GListener.hpp"
#include "megaapi.h"
#include <string>
#include <system_error>
#include <filesystem>
#include <mutex>

namespace {
class CreateFolderListener : public mega::MegaRequestListener {
   public:
	mega::MegaNode *root = nullptr;

	void onRequestStart(mega::MegaApi *api, mega::MegaRequest *request) {
		std::cout << "***** Folder create start" << std::endl;
	}

	void onRequestFinish(mega::MegaApi *api, mega::MegaRequest *request, mega::MegaError *e) override {
		if (e->getErrorCode() != mega::MegaError::API_OK) {
			std::cout << "***** Error creating folder: " << e->getErrorString() << std::endl;
			return;
		}
		std::cout << "***** Folder created successfully" << std::endl;
		delete root;
		api->fetchNodes();
	}

	void onRequestUpdate(mega::MegaApi *api, mega::MegaRequest *request) {
		std::cout << "***** Folder create update" << std::endl;
	}
};

CreateFolderListener create_folder_listener;
} // namespace

void GListener::onRequestFinish(mega::MegaApi *api, mega::MegaRequest *request, mega::MegaError *e) {
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
			std::cout << "***** Showing files/folders in the root folder:" << std::endl;
			mega::MegaNode *root = api->getRootNode();
			mega::MegaNodeList *list = api->getChildren(root);
			bool remote_folder_found = false;
			for (int i = 0; i < list->size(); i++) {
				mega::MegaNode *node = list->get(i);
				if (node->isFile()) continue;
				if (node->getName() == remote_folder) {
					std::cout << "*****   WE FOUND HIM! : ";
					std::cout << node->getName() << std::endl;
					remote_folder_node = node->copy();
					remote_folder_found = true;
					break;
				}
			}
			std::cout << "***** Done" << std::endl;

			delete list;
			if (!remote_folder_found) {
				create_folder_listener.root = root;
				api->createFolder(remote_folder.c_str(), root, &create_folder_listener);
				break;
			}
			if (root) delete root;

			if (zip_files_directory == nullptr) {
				std::cerr << "Directory pointer is null\n";
				finished = true;
				break;
			}

			std::error_code ec;
			if (!std::filesystem::is_directory(*zip_files_directory, ec)) {
				std::cerr << "Not a valid directory: " << *zip_files_directory << (ec ? " (" + ec.message() + ")" : "")
						  << '\n';
				finished = true;
				break;
			}

			const std::lock_guard<std::mutex> lock(counter_mutex);
			for (const std::filesystem::directory_entry &entry :
				 std::filesystem::directory_iterator(*zip_files_directory, ec)) {
				if (entry.is_regular_file()) {
					std::cout << "***** Uploading " << entry.path().filename().string() << '\n';
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
void GListener::onRequestUpdate(mega::MegaApi *, mega::MegaRequest *request) {
	std::cout << "***** Loading filesystem " << request->getTransferredBytes() << " / " << request->getTotalBytes()
			  << std::endl;
}

void GListener::onRequestTemporaryError(mega::MegaApi *, mega::MegaRequest *, mega::MegaError *error) {
	std::cout << "***** Temporary error in request: " << error->getErrorString() << std::endl;
}

void GListener::onTransferFinish(mega::MegaApi *, mega::MegaTransfer *, mega::MegaError *error) {
	if (error->getErrorCode()) {
		std::cout << "***** Transfer finished with error: " << error->getErrorString() << std::endl;
	} else {
		std::cout << "***** Transfer finished OK" << std::endl;
	}
	const std::lock_guard<std::mutex> lock(counter_mutex);
	thread_counter--;
	if (thread_counter == 0) {
		finished = true;
	}
}

void GListener::onTransferUpdate(mega::MegaApi *, mega::MegaTransfer *transfer) {
	std::cout << "***** Transfer progress: " << transfer->getTransferredBytes() << "/" << transfer->getTotalBytes()
			  << std::endl;
}

void GListener::onTransferTemporaryError(mega::MegaApi *, mega::MegaTransfer *, mega::MegaError *error) {
	std::cout << "***** Temporary error in transfer: " << error->getErrorString() << std::endl;
}

void GListener::onUsersUpdate(mega::MegaApi *, mega::MegaUserList *users) {
	if (users == NULL) {
		// Full account reload
		return;
	}
	std::cout << "***** There are " << users->size() << " new or updated users in your account" << std::endl;
}

void GListener::onNodesUpdate(mega::MegaApi *, mega::MegaNodeList *nodes) {
	if (nodes == NULL) {
		// Full account reload
		return;
	}

	std::cout << "***** There are " << nodes->size() << " new or updated node/s in your account" << std::endl;
}

void GListener::onSetsUpdate(mega::MegaApi *, mega::MegaSetList *sets) {
	if (sets) {
		std::cout << "***** There are " << sets->size() << " new or updated Set/s in your account" << std::endl;
	}
}

void GListener::onSetElementsUpdate(mega::MegaApi *, mega::MegaSetElementList *elements) {
	if (elements) {
		std::cout << "***** There are " << elements->size() << " new or updated Set-Element/s in your account"
				  << std::endl;
	}
}
