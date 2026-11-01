#include "GDeleteExcessiveFilesListener.hpp"
#include "megaapi.h"
#include <string>
#include <fmt/base.h>
#include <vector>
#include <cstdio>

void GDeleteExcessiveFilesListener::onRequestFinish(mega::MegaApi *api, mega::MegaRequest *request,
													mega::MegaError *e) {
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
			fmt::println("***** Showing files/folders in the root folder:");
			remote_folder_node = api->getRootNode();
			int remote_folder_found{ 0 };
			for (const auto &directory : *dir_structure) {
				mega::MegaNodeList *list = api->getChildren(remote_folder_node);
				for (int i = 0; i < list->size(); i++) {
					mega::MegaNode *node = list->get(i);
					if (node->isFile()) continue;
					if (node->getName() == directory) {
						fmt::println("*****   WE FOUND HIM! : {}", node->getName());
						delete remote_folder_node;
						remote_folder_node = node->copy();
						remote_folder_found++;
						delete list;
						break;
					}
				}
			}

			if (remote_folder_found < (*dir_structure).size()) {
				finished = true;
				fmt::println(stderr, "Error: Could not find the remote folder structure in MEGA: {}",
							 (*dir_structure)[remote_folder_found]);
				break;
			}

			std::vector<std::string> fileslist;
			mega::MegaNodeList *files = api->getChildren(remote_folder_node);
			for (int i = 0; i < files->size(); i++) {
				mega::MegaNode *node = files->get(i);
				if (node->isFile()) fileslist.emplace_back(node->getName());
			}

			delete files;

			break;
		}
		default: break;
	}
}

// Currently, this callback is only valid for the request fetchNodes()
void GDeleteExcessiveFilesListener::onRequestUpdate(mega::MegaApi *, mega::MegaRequest *request) {
	std::cout << "***** Loading filesystem " << request->getTransferredBytes() << " / " << request->getTotalBytes()
			  << std::endl;
}

void GDeleteExcessiveFilesListener::onRequestTemporaryError(mega::MegaApi *, mega::MegaRequest *,
															mega::MegaError *error) {
	std::cout << "***** Temporary error in request: " << error->getErrorString() << std::endl;
}

void GDeleteExcessiveFilesListener::onTransferFinish(mega::MegaApi *, mega::MegaTransfer *mtransf,
													 mega::MegaError *error) {
	if (error->getErrorCode()) {
		std::cout << "***** Transfer finished with error: " << error->getErrorString() << std::endl;
	} else {
		std::cout << "***** Transfer finished OK: " << mtransf->getPath() << ":" << mtransf->getFileName() << std::endl;
	}
}

void GDeleteExcessiveFilesListener::onTransferUpdate(mega::MegaApi *, mega::MegaTransfer *transfer) {
	std::cout << "***** Transfer progress: " << transfer->getTransferredBytes() << "/" << transfer->getTotalBytes()
			  << std::endl;
}

void GDeleteExcessiveFilesListener::onTransferTemporaryError(mega::MegaApi *, mega::MegaTransfer *,
															 mega::MegaError *error) {
	std::cout << "***** Temporary error in transfer: " << error->getErrorString() << std::endl;
}

void GDeleteExcessiveFilesListener::onUsersUpdate(mega::MegaApi *, mega::MegaUserList *users) {
	if (users == NULL) {
		// Full account reload
		return;
	}
	std::cout << "***** There are " << users->size() << " new or updated users in your account" << std::endl;
}

void GDeleteExcessiveFilesListener::onNodesUpdate(mega::MegaApi *, mega::MegaNodeList *nodes) {
	if (nodes == NULL) {
		// Full account reload
		return;
	}

	std::cout << "***** There are " << nodes->size() << " new or updated node/s in your account" << std::endl;
}

void GDeleteExcessiveFilesListener::onSetsUpdate(mega::MegaApi *, mega::MegaSetList *sets) {
	if (sets) {
		std::cout << "***** There are " << sets->size() << " new or updated Set/s in your account" << std::endl;
	}
}

void GDeleteExcessiveFilesListener::onSetElementsUpdate(mega::MegaApi *, mega::MegaSetElementList *elements) {
	if (elements) {
		std::cout << "***** There are " << elements->size() << " new or updated Set-Element/s in your account"
				  << std::endl;
	}
}
