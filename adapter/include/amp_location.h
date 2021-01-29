/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

/* AMP System Abstract Layer */
#ifndef AMP_LOCATION_H
#define AMP_LOCATION_H

/**
 * Date and time data
 */
typedef struct
{
    unsigned int year; /**< Years                    - [1900, 2089]                 */
    unsigned int mon;  /**< Months                   - [   1,   12]                 */
    unsigned int day;  /**< Day of the month         - [   1,   31]                 */
    unsigned int hour; /**< Hours since midnight     - [   0,   23]                 */
    unsigned int min;  /**< Minutes after the hour   - [   0,   59]                 */
    unsigned int sec;  /**< Seconds after the minute - [   0,   60] (1 leap second) */
    unsigned int hsec; /**< Hundredth part of second - [   0,   99]                 */
} amp_nmea_time_t;

/**
 * GPS information from all supported sentences
 */
typedef struct
{
    unsigned int    present;    /**< Bit-mask specifying which fields are present                    */
    unsigned int    smask;      /**< Bit-mask specifying from which sentences data has been obtained */
    amp_nmea_time_t utc;        /**< UTC of the position data                                        */
    double          pdop;       /**< Position Dilution Of Precision                                  */
    double          hdop;       /**< Horizontal Dilution Of Precision                                */
    double          vdop;       /**< Vertical Dilution Of Precision                                  */
    double          latitude;   /**< Latitude,  in NDEG: +/-[degree][min].[sec/60]                   */
	unsigned char   latitude_cardinal; /* N or S */
    double          longitude;  /**< Longitude, in NDEG: +/-[degree][min].[sec/60]                   */
	unsigned char   longitude_cardinal; /* E or W */
    double          elevation;  /**< Elevation above/below mean sea level (geoid), in meters海拔     */
    double          height;     /**< Height of geoid (elevation) above WGS84 ellipsoid, in meters    */
    double          speed;      /**< Speed over the ground in kph                                    */
    double          track;      /**< Track angle in degrees true north                               */
    double          mtrack;     /**< Magnetic Track angle in degrees true north                      */
    double          magvar;     /**< Magnetic variation in degrees                                   */
    double          dgpsAge;    /**< Time since last DGPS update, in seconds                         */
    unsigned int    dgpsSid;    /**< DGPS station ID number                                          */
    unsigned char   metric;     /**< When true then units are metric                        */
} amp_nmea_info_t;

typedef enum
{
	AMP_GNSS_ALL,
	AMP_GNSS_RTCM,
	AMP_GNSS_NMEA,
	AMP_GNSS_UNKN,
}AMP_DATA_TYPE_E;

typedef void (* amp_gnss_event)(AMP_DATA_TYPE_E type, unsigned char *data, int len);

/**
 * @brief amp_gnss_init() deletes a directory, which must be emtpy.
 *
 * @param[in] event  The directory to be deleted.
 *
 * @return  On success, return 0.
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
int amp_gnss_init(amp_gnss_event event);

/**
 * @brief amp_gnss_deinit() deletes a directory, which must be emtpy.
 *
 * @return  On success, return 0.
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
int amp_gnss_deinit(void);

/**
 * @brief amp_gnss_open() deletes a directory, which must be emtpy.
 *
 * @return  On success, return 0.
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
int amp_gnss_open(void);

/**
 * @brief amp_gnss_close() deletes a directory, which must be emtpy.
 *
 * @return  On success, return 0.
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
int amp_gnss_close(void);

/**
 * @brief amp_gnss_sleep() deletes a directory, which must be emtpy.
 *
 * @return  On success, return 0.
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
int amp_gnss_sleep(void);

/**
 * @brief amp_gnss_location_info_get() deletes a directory, which must be emtpy.
 *
 * @param[in] info  The directory to be deleted.
 *
 * @return  On success, return 0.
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
int amp_gnss_location_info_get(amp_nmea_info_t *info);

#endif /* AMP_LOCATION_H */