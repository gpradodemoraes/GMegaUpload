#include "GListener.hpp"
#include "megaapi.h"

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

			for (int i = 0; i < list->size(); i++) {
				mega::MegaNode *node = list->get(i);
				if (node->isFile())
					std::cout << "*****   File:   ";
				else
					std::cout << "*****   Folder: ";

				std::cout << node->getName() << std::endl;
			}
			std::cout << "***** Done" << std::endl;

			delete list;

			std::cout << "***** Uploading the image MEGA.png" << std::endl;

			mega::MegaUploadOptions uploadOptions;
			uploadOptions.mtime = 0;
			api->startUpload(std::string{ "MEGA.png" }, root, nullptr, &uploadOptions, nullptr);

			delete root;

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

	finished = true;
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
