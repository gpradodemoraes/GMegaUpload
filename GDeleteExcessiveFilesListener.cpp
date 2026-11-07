#include "GDeleteExcessiveFilesListener.hpp"
#include "megaapi.h"
#include <string>
#include <vector>
#include <cstdio>
#include <regex>
#include <map>
#include <spdlog/spdlog.h>

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
		case mega::MegaRequest::TYPE_REMOVE: {
			delete_file_counter.fetch_sub(1, std::memory_order_acquire);
			if (delete_file_counter.load() == 0) finished = true;
			break;
		}
		case mega::MegaRequest::TYPE_FETCH_NODES: {
			SPDLOG_INFO("***** Showing files/folders in the root folder:");
			remote_folder_node = api->getRootNode();
			int remote_folder_found{ 0 };
			for (const auto &directory : *dir_structure) {
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

			if (remote_folder_found < (*dir_structure).size()) {
				finished = true;
				SPDLOG_ERROR("Error: Could not find the remote folder structure in MEGA: {}",
							 (*dir_structure)[remote_folder_found]);
				break;
			}

			std::vector<std::string> filesList;
			mega::MegaNodeList *files = api->getChildren(remote_folder_node);
			for (int i = 0; i < files->size(); i++) {
				mega::MegaNode *node = files->get(i);
				if (node->isFile()) filesList.emplace_back(node->getName());
			}
			delete files;

			std::map<std::string, std::map<std::string, std::vector<std::string>>> ordered_results;
			std::regex regex_pattern(R"((\d{4}-\d{2}-\d{2}-\d{6}))");
			for (const auto &file : filesList) {
				for (const auto &[key, value] : *excessive_files_rules) {
					if (file.rfind(key, 0) == 0) {
						std::smatch match;
						if (std::regex_search(file, match, regex_pattern)) {
							if (ordered_results.find(key) == ordered_results.end()) {
								ordered_results[key] = std::map<std::string, std::vector<std::string>>();
								ordered_results[key].insert({ match[1], { file } });
							} else {
								ordered_results[key][match[1]].push_back(file);
							}
						}
					}
				}
			}

			for (const auto &[key, max_lists_to_keep] : *excessive_files_rules) {
				int count{ 0 };
				int size = static_cast<int>(ordered_results[key].size());
				for (const auto &[k, list_of_files] : ordered_results[key]) {
					if (count < size - max_lists_to_keep) {
						for (const auto &file : list_of_files) {
							SPDLOG_INFO("DELETE THIS: {}", file);
							delete_file_counter.fetch_add(1, std::memory_order_acquire);
							api->remove(api->getChildNode(remote_folder_node, file.c_str()));
						}
						count++;
					}
				}
			}
			if (delete_file_counter.load() == 0) finished = true;
			break;
		}
		default: break;
	}
}

// Currently, this callback is only valid for the request fetchNodes()
void GDeleteExcessiveFilesListener::onRequestUpdate(mega::MegaApi *, mega::MegaRequest *request) {
	SPDLOG_INFO("***** Loading filesystem {}: {} / {}", request->getRequestString(), request->getTransferredBytes(),
				request->getTotalBytes());
}

void GDeleteExcessiveFilesListener::onRequestTemporaryError(mega::MegaApi *, mega::MegaRequest *,
															mega::MegaError *error) {
	SPDLOG_INFO("***** Temporary error in request: {}", error->getErrorString());
}

void GDeleteExcessiveFilesListener::onTransferFinish(mega::MegaApi *, mega::MegaTransfer *mtransf,
													 mega::MegaError *error) {
	if (error->getErrorCode()) {
		SPDLOG_INFO("***** Transfer finished with error: {}", error->getErrorString());
	} else {
		SPDLOG_INFO("***** Transfer finished OK: {}: {}", mtransf->getPath(), mtransf->getFileName());
	}
}

void GDeleteExcessiveFilesListener::onTransferUpdate(mega::MegaApi *, mega::MegaTransfer *transfer) {
	SPDLOG_INFO("***** Transfer progress: {} / {}", transfer->getTransferredBytes(), transfer->getTotalBytes());
}

void GDeleteExcessiveFilesListener::onTransferTemporaryError(mega::MegaApi *, mega::MegaTransfer *,
															 mega::MegaError *error) {
	SPDLOG_INFO("***** Temporary error in transfer: {}", error->getErrorString());
}

void GDeleteExcessiveFilesListener::onUsersUpdate(mega::MegaApi *, mega::MegaUserList *users) {
	if (users == NULL) {
		// Full account reload
		return;
	}
	SPDLOG_INFO("***** There are {} new or updated users in your account", users->size());
}

void GDeleteExcessiveFilesListener::onNodesUpdate(mega::MegaApi *, mega::MegaNodeList *nodes) {
	if (nodes == NULL) {
		// Full account reload
		return;
	}

	SPDLOG_INFO("***** There are {} new or updated node/s in your account", nodes->size());
}

void GDeleteExcessiveFilesListener::onSetsUpdate(mega::MegaApi *, mega::MegaSetList *sets) {
	if (sets) {
		SPDLOG_INFO("***** There are {} new or updated Set/s in your account", sets->size());
	}
}

void GDeleteExcessiveFilesListener::onSetElementsUpdate(mega::MegaApi *, mega::MegaSetElementList *elements) {
	if (elements) {
		SPDLOG_INFO("***** There are {} new or updated Set-Element/s in your account", elements->size());
	}
}
