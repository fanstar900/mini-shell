#pragma once

#include<string>
#include "pipeline.h"

class Executor{

    public:
        void execute(const Pipeline& pipeline) ;

    private:
        void executeSingleCommand(const Command& command);
        void executePipeline(const Pipeline& pipeline);
        void executeSingleCommandHelper(const Command& command) ;
        void executeMultiPipeline(const Pipeline& pipeline) ;

} ;