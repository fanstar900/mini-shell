#pragma once

#include<string>
#include "pipeline.h"
#include "job_manager.h"

class Executor{

    public:
        void execute(const Pipeline& pipeline, JobManager& jobManager) ;
    private:
        void executeSingleCommand(const Command& command, bool background, JobManager& jobManager);
        void executePipeline(const Pipeline& pipeline);
        void executeSingleCommandHelper(const Command& command) ;
        void executeMultiPipeline(const Pipeline& pipeline) ;

} ;