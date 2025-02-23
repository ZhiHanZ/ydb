#include "response_tasks.h"

namespace NYq {

void TResponseTasks::AddTaskNonBlocking(const TString& key, const TEvControlPlaneStorage::TTask& task) {
    Tasks[key] = task;
}

void TResponseTasks::AddTaskBlocking(const TString& key, const TEvControlPlaneStorage::TTask& task) {
    with_lock (Mutex) {
        Tasks[key] = task;
    }
}

void TResponseTasks::SafeEraseTaskNonBlocking(const TString& key) {
    if (auto it = Tasks.find(key); it != Tasks.end())
        Tasks.erase(it);
}

void TResponseTasks::SafeEraseTaskBlocking(const TString& key) {
    with_lock (Mutex) {
        if (auto it = Tasks.find(key); it != Tasks.end())
            Tasks.erase(it);
    }
}

bool TResponseTasks::EmptyNonBlocking() {
    return Tasks.empty();
}

bool TResponseTasks::EmptyBlocking() {
    with_lock (Mutex) {
        return Tasks.empty();
    }
}

const THashMap<TString, TEvControlPlaneStorage::TTask>& TResponseTasks::GetTasksNonBlocking() {
    return Tasks;
}

const THashMap<TString, TEvControlPlaneStorage::TTask>& TResponseTasks::GetTasksBlocking() {
    with_lock (Mutex) {
        return Tasks;
    }
}

} //NYq