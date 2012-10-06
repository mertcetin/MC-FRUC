/*
 * Header for rand31-park-miller-carta-int.c
 */ 
                                /* Function declarations
                                 */

    long unsigned int rand31pmc_next(void);
    void              rand31pmc_seedi(long unsigned int);
    long unsigned int rand31pmc_ranlui(void);  
    float             rand31pmc_ranf(void);

                                /* The sole item of state for each generator:
                                 * a 32 bit integer.
                                 *
                                 * Initialise it to 1 in case the user doesn't
                                 * call seedi().  The range of allowable values
                                 * is 1 to 2^31-1.  A seed value of 0 will cause
                                 * the generation of all zeros.
                                 */
                                 
    long unsigned int seed31pmc  = 1;   
        
    
