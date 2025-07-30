/**
# Copyright (c) @ 2022-2025 OpenZI 数化软件, All rights reserved.
#
# Licensed under GNU AFFERO GENERAL PUBLIC LICENSE VERSION 3, (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.gnu.org/licenses/agpl-3.0.en.html#license-text
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################
*/

#pragma once

#include <queue>
#include <mutex>

namespace OpenZI
{
    template<typename ItemType, typename Container = std::queue<ItemType>>
    class TQueue
    {
    public:
        TQueue() = default;
        ~TQueue() { Empty(); };


        bool Dequeue(ItemType& OutItem)
        {
            std::scoped_lock Lock(Mutex);
            if (Queue.empty())
                return false;
            OutItem = std::move_if_noexcept(Queue.front());
            Queue.pop();
            return true;
        }

        void Empty()
        {
            std::scoped_lock Lock(Mutex);
            for (int i = 0; i < Queue.size(); ++i)
            {
                Queue.pop();
            }
        }

        void Enqueue(const ItemType& Item)
        {
            SafePush(Item);
        }

        void Enqueue(ItemType&& Item)
        {
            SafePush(std::move(Item));
        }

        bool IsEmpty() const
        {
            std::scoped_lock Lock(Mutex);
            return Queue.empty();
        }

        inline void SafePush(ItemType&& Item)
        {
            std::scoped_lock Lock(Mutex);
            Queue.emplace(std::move(Item));
        }

        inline void SafePush(const ItemType& Item) {
            std::scoped_lock Lock(Mutex);
            Queue.emplace(Item);
        }

        size_t Num() const
        {
            std::scoped_lock Lock(Mutex);
            return Queue.size();
        }
    private:

        Container Queue;
        mutable std::mutex Mutex;

        TQueue(const TQueue&) = delete;
        TQueue(TQueue&&) = delete;
        TQueue& operator=(const TQueue&) = delete;
        TQueue& operator=(TQueue&&) = delete;
    };
} // namespace OpenZI