#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

namespace Afina {
namespace Coroutine {

void Engine::Store(context &ctx) {
    char tmp;
    ctx.Low = &tmp;
    std::size_t stack_size = ctx.Hight - ctx.Low;
    if (std::get<1>(ctx.Stack) < stack_size || std::get<1>(ctx.Stack) > stack_size * 2){
        delete[] std::get<0>(ctx.Stack);
        std::get<1>(ctx.Stack) = stack_size;
        std::get<0>(ctx.Stack) = new char[stack_size];
    }
    memcpy(std::get<0>(ctx.Stack), ctx.Low, stack_size);
}

void Engine::Restore(context &ctx) {
    char tmp;
    while(&tmp >= ctx.Low && &tmp <= ctx.Hight){
        Restore(ctx);
    }
    memcpy(ctx.Low, std::get<0>(ctx.Stack), ctx.Hight - ctx.Low);
    cur_routine = &ctx;
    longjmp(ctx.Environment, 1);
}

void Engine::yield() {
    context* coro = alive;
    while(coro == cur_routine && coro != nullptr){
        coro = coro->next;
    }
    if (coro == nullptr){
        return;
    }
    else {
        sched(coro);
    }
}

void Engine::sched(void *routine_) {
    if (routine_ == nullptr){
        yield();
        return;
    }

    context* coro = static_cast<context*>(routine_);

    if(coro == cur_routine){
        return;
    }

    if (cur_routine != idle_ctx){
        if(setjmp(cur_routine->Environment) > 0){
            return;
        }
        Store(*cur_routine);
    }
    cur_routine = coro;
    Restore(*cur_routine);
}

void Engine::block(void *coro){
    if(coro == nullptr || coro == cur_routine){
        delete_from(alive, cur_routine);
        add_to(blocked, cur_routine);
        yield();
    }
    else {
        context* routine_ = static_cast<context*>(coro);
        delete_from(alive, routine_);
        add_to(blocked, routine_);
    }
}

void Engine::unblock(void *coro){
    if(coro == nullptr){
        return;
    }
    
    context* routine_ = static_cast<context*>(coro);
    delete_from(blocked, routine_);
    add_to(alive, routine_);
}

void Engine::delete_from(context*& list, context*& routine_){
    if (list == nullptr){
        return;
    }
    if (list == routine_){
        list = routine_->next;
    }
    if(routine_->next != nullptr){
        routine_->next->prev = routine_->prev;
    }
    if(routine_->prev != nullptr){
        routine_->prev->next = routine_->next;
    }
}
void Engine::add_to(context*& list, context*& routine_){
    if(list == nullptr){
        list = routine_;
        list->next = nullptr;
        list->prev = nullptr;
    }
    else {
        routine_->next = list;
        routine_->prev = nullptr;
        list->prev = routine_;
        list = routine_;
    }
}

} // namespace Coroutine
} // namespace Afina
