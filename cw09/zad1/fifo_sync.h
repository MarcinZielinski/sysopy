//
// Created by Mrz355 on 21.05.17.
//

#ifndef CW09_FIFO_SYNC_H
#define CW09_FIFO_SYNC_H

#define WRITER 0
#define READER 1

struct reader_args {
    int divider;
    int id;
};

struct writer_args {
    int id;
};

#endif //CW09_FIFO_SYNC_H
