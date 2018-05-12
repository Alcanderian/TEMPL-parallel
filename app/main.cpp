//
// Created by Alcan on 2018/5/12.
//
#include <iostream>

#include "../src/args.h"
#include "../src/strings.h"
#include "../src/parallel.h"

void outId(ThreadArgument &arg) {
    printf("My id is %d\n", arg.thread_id);
}

int main(int argc, char **argv) {
    IOParser parser;

    parser.setCommandLine(argc, argv);

    parser.addSection("My Test Section");
    int i = textToInteger(parser.getOption("--j", "Threads to use", "NULL"));

    if(parser.checkForErrors())
        return 0;

    ThreadManager manager(i);
    manager.run(outId);
}
