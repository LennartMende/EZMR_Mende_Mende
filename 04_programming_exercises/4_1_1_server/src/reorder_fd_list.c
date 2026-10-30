#include "server_utils.h"



// // detect gaps in the fd list 
// // and rearange the list if gaps are there
void reorder_list(int peer_fds[MAX_CLIENT_COUNT]) {
    int print = 0;
    while (1) {
        // detect non neg fd entries and their indices
        int non_negative_fd_idxs[MAX_CLIENT_COUNT];
        int non_neg_incoming_fds_count = detect_gaps_in_fds(peer_fds, non_negative_fd_idxs, print);

        // detect gaps and their indices and count them
        int gap_list_idxs[non_neg_incoming_fds_count];
        int gap_count = has_gaps(peer_fds, non_neg_incoming_fds_count, gap_list_idxs, print);
        
        
        if (gap_count == 0) { // no gaps -> no changes
            if (print) {
                printf("fd list is in the right order.\n");
            }
            break;
        }

        print = 1;

        // rearange the array to get rid of the gaps
        int changes = rearange_peer_fd_list(peer_fds, non_negative_fd_idxs, 
            non_neg_incoming_fds_count, gap_list_idxs, gap_count, print);
        
        if (print) {
            printf("There happened %d changes when rearanging.", changes);
        }

        printf("Stuck in loop! The first 10 peer_fds are: [");
        for (int peer_fd_idx = 0; peer_fd_idx < 9; peer_fd_idx++) {
            printf("%d, ", peer_fds[peer_fd_idx]);
        }
        printf("%d]\n", peer_fds[9]);
    }
}


// returns 0 if no non negative peer_fds, 
// otherwise number of non negative peer_fds
// can be applied on any peer_fd_list (incoming or outgoing)
int detect_gaps_in_fds(int peer_fd_list[MAX_CLIENT_COUNT], int non_negative_fd_idxs[MAX_CLIENT_COUNT], int print) {
    // check if incoming fds are sorted
    int non_negative_fd_idxs_idx = 0; // holds idxs of non negative entries from 0 to non_negative_fds_count_count-1
    for (int i = 0; i < MAX_CLIENT_COUNT; i++) {
        if (peer_fd_list[i] != -1) {
            if (peer_fd_list[i] < -1) {
                fprintf(stderr, "FILE FD < -1 SHOULD NOT BE POSSIBLE");
            } else {
                non_negative_fd_idxs[non_negative_fd_idxs_idx] = i;
                non_negative_fd_idxs_idx++;
            }
        }
    }
    if (non_negative_fd_idxs_idx != 0) {
        if (print) {
            printf("\n%d non negative fds, their indexes in the lists are:\n[", non_negative_fd_idxs_idx);
        }
        for (int i = 0; i < non_negative_fd_idxs_idx; i++) {
            if (print) {
                printf("%d", non_negative_fd_idxs[i]);
            }
            if (i < non_negative_fd_idxs_idx - 1 && print) {
                printf(", ");
            }
        }
        if (print) {
            printf("]\n");
        }
    } else {
        if (print) {
            printf("No non negative fds available.\n");
        }
    }
    
    return non_negative_fd_idxs_idx; // returns 0 if no non negative peer_fds, otherwise number of non negative peer_fds
}


// checks if there are gaps, meaning a -1 before a non negative number
// returns if there were gaps
int has_gaps(int peer_fd_list[MAX_CLIENT_COUNT], 
        int non_negative_fd_count, int gap_list_idxs[non_negative_fd_count], int print) { // non_negative_fd_count entries are enough for non_negative_fd_count peers
    int gaps = 0;
    for (int fd_idx = 0; fd_idx < non_negative_fd_count; fd_idx++) {
        if (peer_fd_list[fd_idx] < 0) {
            gap_list_idxs[gaps] = fd_idx;
            gaps++;
        }
    }

    if (gaps && print) {
        printf("There are %d gaps at the following indices:\n[", gaps);
        for (int gap_idx = 0; gap_idx < gaps; gap_idx++) {
            printf("%d", gap_list_idxs[gap_idx]);
            if (gap_idx < gaps - 1) {
                printf(", ");
            }
        }
        printf("]\n");
    } else {
        if (print) {
            printf("List has no gaps\n");
        }
    }
    return gaps;
}


// returns 0 if no rearanging occured, else number of rearangements
int rearange_peer_fd_list(int peer_fd_list[MAX_CLIENT_COUNT], int non_negative_fd_idxs[MAX_CLIENT_COUNT], 
        int non_negative_fd_count, int gap_list_idxs[non_negative_fd_count], int gap_count, int print) {
    int changes = 0; 
    if (gap_count < 0) {
        if (print) {
            printf("Error: Gap count cannot be negative!\n\n");
        }
        changes = -1; // as error code
    } else if (gap_count == 0) { // else no changes were required so no rearanging occured
        if (print) {
            printf("No rearangement required because there are no gaps.\n\n");
        }
        changes = 0;
    } else { // rearanging has to take place#
        printf("has_gaps = %d, gaps are at indices [", gap_count);
        for (int gap_list_idxs_idx = 0; gap_list_idxs_idx < gap_count - 1; gap_list_idxs_idx++) {
            printf("%d, ", gap_list_idxs[gap_list_idxs_idx]);
        }
        printf("%d]\n", gap_list_idxs[gap_count - 1]);
        // printf("peer_fd list before rearanging: [");
        // for (int i = 0; i < 10; i++) {
        //     if (i < 9) {
        //         printf("%d, ", peer_fd_list[i]);
        //     }
        // }
        // printf("%d]\n", peer_fd_list[9]);
        printf("\nStuck in loop! The first 10 peer_fds before rearanging are: [");
        for (int peer_fd_idx = 0; peer_fd_idx < 9; peer_fd_idx++) {
            printf("%d, ", peer_fd_list[peer_fd_idx]);
        }
        printf("%d]\n", peer_fd_list[9]);
        for (int gap_number = 0; gap_number < gap_count; gap_number++) {
            printf("gap_number = %d, non_negative_fd_idxs[gap_number] = %d, peer_fd_list[non_negative_fd_idxs[gap_number]] = %d\n", 
                gap_number, non_negative_fd_idxs[gap_number], peer_fd_list[non_negative_fd_idxs[gap_number]]);
            printf("gap_list_idxs[gap_number] = %d\n", gap_list_idxs[gap_number]);
            printf("peer_fd_list[gap_list_idxs[gap_number]] = %d\n", peer_fd_list[gap_list_idxs[gap_number]]);

            
            
            printf("non_negative_fd_idxs = [");
            for (int non_negative_fd_idxs_idx = 0; non_negative_fd_idxs_idx < non_negative_fd_count - 1; non_negative_fd_idxs_idx++) {
                printf("%d, ", non_negative_fd_idxs[non_negative_fd_idxs_idx]);
            }
            printf("%d]\n", non_negative_fd_idxs[non_negative_fd_count - 1]);
            
            /*
            Algorithm to rearange:
            Where is the gap_number+1th gap? -> gap_list_idxs[gap_number]
            take it's position: gap_list_idxs[gap_number] -> the element is peer_fd_list[gap_list_idxs[gap_number]] and element == -1
            now look for the next biggest number in non_neg_fd_idxs_idx that's a bit bigger than the gap index (gap_list_idxs[gap_number])
            save this index in next_non_neg_fd_idx: it is the index of the fd that has to be swapped into the previous gap
            */
            int next_non_neg_fd_idx = -1;

            for (int non_neg_fd_idxs_idx = 0; non_neg_fd_idxs_idx < non_negative_fd_count; non_neg_fd_idxs_idx++) {
                if (non_negative_fd_idxs[non_neg_fd_idxs_idx] > gap_list_idxs[gap_number]) {
                    next_non_neg_fd_idx = non_negative_fd_idxs[non_neg_fd_idxs_idx];
                    break;
                }
            }

            printf("next_non_neg_fd_idx = %d\n", next_non_neg_fd_idx);
            printf("peer_fd_list[next_non_neg_fd_idx] = %d", peer_fd_list[next_non_neg_fd_idx]);
            
            peer_fd_list[gap_list_idxs[gap_number]] = peer_fd_list[next_non_neg_fd_idx];
            peer_fd_list[next_non_neg_fd_idx] = -1;
            changes++;
        }
        printf("Stuck in loop! peer_fd list after rearanging: [");
        for (int i = 0; i < 10; i++) {
            if (i < 9) {
                printf("%d, ", peer_fd_list[i]);
            }
        }
        printf("%d]\n\n", peer_fd_list[9]);
    }
    return changes;
}
