/*
    LPCUSB, an USB device driver for LPC microcontrollers   
    Copyright (C) 2006 Bertrik Sikken (bertrik@sikken.nl)

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. The name of the author may not be used to endorse or promote products
       derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, 
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    Modified by Sivan Toledo.
*/


/******************************************************/
/* USB Hardware Layer                                 */
/******************************************************/

/**
  Hardware definitions for the LPC214x USB controller
  These are private to the usbhw module
*/

/* USBDevInt... bits */
#define FRAME             (1<<0)
#define EP_FAST           (1<<1)
#define EP_SLOW           (1<<2)
#define DEV_STAT          (1<<3)
#define CCEMTY            (1<<4)
#define CDFULL            (1<<5)
#define RxENDPKT          (1<<6)
#define TxENDPKT          (1<<7)
#define EP_RLZED          (1<<8)
#define ERR_INT           (1<<9)

/* USBRxPLen bits */
#define PKT_LNGTH         (1<<0)
#define PKT_LNGTH_MASK        0x3FF
#define DV              (1<<10)
#define PKT_RDY           (1<<11)

/* USBCtrl bits */
#define RD_EN           (1<<0)
#define WR_EN           (1<<1)
#define LOG_ENDPOINT        (1<<2)

/* protocol engine command codes */
  /* device commands */
#define CMD_DEV_SET_ADDRESS     0xD0
#define CMD_DEV_CONFIG        0xD8
#define CMD_DEV_SET_MODE      0xF3
#define CMD_DEV_READ_CUR_FRAME_NR 0xF5
#define CMD_DEV_READ_TEST_REG   0xFD
#define CMD_DEV_STATUS        0xFE    /* read/write */
#define CMD_DEV_GET_ERROR_CODE    0xFF
#define CMD_DEV_READ_ERROR_STATUS 0xFB
  /* endpoint commands */
#define CMD_EP_SELECT       0x00
#define CMD_EP_SELECT_CLEAR     0x40
#define CMD_EP_SET_STATUS     0x40
#define CMD_EP_CLEAR_BUFFER     0xF2
#define CMD_EP_VALIDATE_BUFFER    0xFA

/* set address command */
#define DEV_ADDR          (1<<0)
#define DEV_EN            (1<<7)

/* configure device command */
#define CONF_DEVICE         (1<<0)

/* set mode command */
#define AP_CLK            (1<<0)
#define INAK_CI           (1<<1)
#define INAK_CO           (1<<2)
#define INAK_II           (1<<3)
#define INAK_IO           (1<<4)
#define INAK_BI           (1<<5)
#define INAK_BO           (1<<6)

/* set get device status command */
#define CON             (1<<0)
#define CON_CH            (1<<1)
#define SUS             (1<<2)
#define SUS_CH            (1<<3)
#define RST             (1<<4)

/* get error code command */
// ...

/* Select Endpoint command read bits */
#define EPSTAT_FE         (1<<0)
#define EPSTAT_ST         (1<<1)
#define EPSTAT_STP          (1<<2)
#define EPSTAT_PO         (1<<3)
#define EPSTAT_EPN          (1<<4)
#define EPSTAT_B1FULL       (1<<5)
#define EPSTAT_B2FULL       (1<<6)

/* CMD_EP_SET_STATUS command */
#define EP_ST           (1<<0)
#define EP_DA           (1<<5)
#define EP_RF_MO          (1<<6)
#define EP_CND_ST         (1<<7)

/* read error status command */
#define PID_ERR           (1<<0)
#define UEPKT           (1<<1)
#define DCRC            (1<<2)
#define TIMEOUT           (1<<3)
#define EOP             (1<<4)
#define B_OVRN            (1<<5)
#define BTSTF           (1<<6)
#define TGL_ERR           (1<<7)

/***********************************/
#ifdef DEBUG
// comment out the following line if you don't want to use debug LEDs
#define DEBUG_LED
#endif

#ifdef DEBUG_LED
#define DEBUG_LED_ON(x)   IOCLR0 = (1 << x);
#define DEBUG_LED_OFF(x)  IOSET0 = (1 << x);
#define DEBUG_LED_INIT(x) PINSEL0 &= ~(0x3 << (2*x)); IODIR0 |= (1 << x); DEBUG_LED_OFF(x);
#else
#define DEBUG_LED_INIT(x) /**< LED initialisation macro */
#define DEBUG_LED_ON(x)   /**< turn LED on */
#define DEBUG_LED_OFF(x)  /**< turn LED off */
#endif

#define EP2IDX(ep) ((((ep)&0xF)<<1)|(((ep)&0x80)>>7))
#define IDX2EP(idx) ((((idx)<<7)&0x80)|(((idx)>>1)&0xF))

static void _usbWaitForDeviceIntFlags(uint32_t flags) {
  while ((USBDevIntSt & flags) != flags);
  USBDevIntClr = flags; /* now clear */
}

static void _usbCommand(uint8_t cmd) {
  USBDevIntClr = CDFULL | CCEMTY;
  USBCmdCode = 0x00000500     /* command phase */
               | (cmd << 16);
  _usbWaitForDeviceIntFlags(CCEMTY);
}

static void _usbCommandAndWrite(uint8_t cmd, uint16_t data) {
  _usbCommand(cmd); /* command phase */
  
  USBCmdCode = 0x00000100  /* write data phase */
               | (data << 16);
  _usbWaitForDeviceIntFlags(CCEMTY);
}

static uint8_t _usbCommandAndRead(uint8_t cmd) {
  _usbCommand(cmd);
  
  USBCmdCode = 0x00000200 /* read phase */ 
               | (cmd << 16);
  _usbWaitForDeviceIntFlags(CDFULL);
  return USBCmdData;
}

static void _usbConfigureEP(uint8_t ep, uint16_t max_packet_size) {
  int idx;
  
  idx = EP2IDX(ep);

  USBReEP |= (1 << idx);
  USBEpInd = idx;
  USBMaxPSize = max_packet_size;
  _usbWaitForDeviceIntFlags(EP_RLZED);

  USBEpIntEn |= (1 << idx);
  USBDevIntEn |= EP_SLOW;

  /* enable; don't need it, this is the reset value */
  //_usbCommandAndWrite(CMD_EP_SET_STATUS | idx, 0); 
}

static inline void _usbSetAddress(uint8_t addr) {
  _usbCommandAndWrite(CMD_DEV_SET_ADDRESS, DEV_EN | addr);
}

static inline void usbConnect() {
  _usbCommandAndWrite(CMD_DEV_STATUS, CON);
}

static inline void usbDisconnect(bool connect) {
  _usbCommandAndWrite(CMD_DEV_STATUS, 0);
}

/**
   Enables interrupt on NAK condition
    
   For IN endpoints a NAK is generated when the host wants to read data
   from the device, but none is available in the endpoint buffer.
   For OUT endpoints a NAK is generated when the host wants to write data
   to the device, but the endpoint buffer is still full.
   
   The endpoint interrupt handlers can distinguish regular (ACK) interrupts
   from NAK interrupt by checking the bits in their bEPStatus argument.
   
   @param [in]  bIntBits  Bitmap indicating which NAK interrupts to enable
 */
static void usbEnableNAKInterrupts(uint8_t bIntBits) {
  _usbCommandAndWrite(CMD_DEV_SET_MODE, bIntBits);
}

static uint8_t _usbGetEPStatus(uint8_t ep) {
  int idx = EP2IDX(ep);

  return _usbCommandAndRead(CMD_EP_SELECT | idx);
}

static void _usbStallEP(uint8_t ep) {
  int idx = EP2IDX(ep);
  
  _usbCommandAndWrite(CMD_EP_SET_STATUS | idx, EP_ST);
}

static void _usbUnstallEP(uint8_t ep) {
  int idx = EP2IDX(ep);
  
  _usbCommandAndWrite(CMD_EP_SET_STATUS | idx, 0);
}

static void usbWrite(uint8_t ep, uint8_t *buffer, int len) {
  int idx;
  
  idx = EP2IDX(ep);
  
  USBCtrl = WR_EN | ((ep & 0xF) << 2); /* enable writes */
  USBTxPLen = len;                      /* packet length */
  
  while (USBCtrl & WR_EN) {             /* write data    */
    USBTxData = (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
    buffer += 4;
  }
  
  // select endpoint and validate buffer
  _usbCommand(CMD_EP_SELECT | idx);
  _usbCommand(CMD_EP_VALIDATE_BUFFER);
  
  //return len;
}

/**
   Reads data from an endpoint buffer
   
   @param [in]  bEP   Endpoint number
   @param [in]  pbBuf Endpoint data
   @param [in]  iMaxLen Maximum number of bytes to read
   
   @return the number of bytes available in the EP (possibly more than iMaxLen),
           or <0 in case of error.
*/
static int usbRead(uint8_t ep, uint8_t *buffer, int buffer_len)
{
  int i, idx;
  uint32_t  data, len;
  
  idx = EP2IDX(ep);
  
  // set read enable bit for specific endpoint
  USBCtrl = RD_EN | ((ep & 0xF) << 2);
  
  // wait for PKT_RDY
  do {
    len = USBRxPLen;
  } while ((len & PKT_RDY) == 0);
  
  // packet valid?
  if ((len & DV) == 0) {
    return -1;
  }
  
  // get length
  len &= PKT_LNGTH_MASK;
  
  // get data
  data = 0;
  for (i = 0; i < len; i++) {
    if ((i % 4) == 0) {
      data = USBRxData;
    }
    if ((buffer != NULL) && (i < buffer_len)) {
      buffer[i] = data & 0xFF;
    }
    data >>= 8;
  }
  
  // make sure RD_EN is clear
  USBCtrl = 0;
  
  // select endpoint and clear buffer
  _usbCommand(CMD_EP_SELECT | idx);
  _usbCommand(CMD_EP_CLEAR_BUFFER);
  
  return len;
}

/**
   Sets the 'configured' state.
    
   All registered endpoints are 'realised' and enabled, and the
   'configured' bit is set in the device status register.
   
   @param [in]  fConfigured If TRUE, configure device, else unconfigure
*/
static inline void usbConfigureDevice() {
  _usbCommandAndWrite(CMD_DEV_CONFIG, CONF_DEVICE);
}

static inline void usbUnconfigureDevice() {
  _usbCommandAndWrite(CMD_DEV_CONFIG, 0);
}

/*
   USB interrupt handler 
   @todo Get all 11 bits of frame number instead of just 8
   Endpoint interrupts are mapped to the slow interrupt
 */
static void 
  __attribute__ ((interrupt("IRQ")))
  _usbIsr(void) 
{
  uint32_t  dwStatus;
  uint32_t dwIntBit;
  uint8_t bEPStat, bDevStat, bStat;
  int i;
  uint16_t  wFrame;
  
  DEBUG_LED_ON(9); // LED9 monitors total time in interrupt routine

  dwStatus = USBDevIntSt; /* record interrupt status */ 
  
  if (dwStatus & FRAME) { /* frame interrupt */
    USBDevIntClr = FRAME; /* clear interrupt flag */
    usb_SOF_handler();
    //if (_pfnFrameHandler != NULL) {
      // wFrame = _usbCommandAndRead(CMD_DEV_READ_CUR_FRAME_NR); /* reads only 8 of 11 bits! */
      //_pfnFrameHandler();
    //}
  }
  
  if (dwStatus & DEV_STAT) { /* device status */
    /* 
     * Clear DEV_STAT interrupt before reading DEV_STAT register.
     * This prevents corrupted device status reads, see
     * LPC2148 User manual revision 2, 25 july 2006.
     */
    USBDevIntClr = DEV_STAT;
    bDevStat = _usbCommandAndRead(CMD_DEV_STATUS);
    if (bDevStat & (CON_CH | SUS_CH | RST)) {
      // convert device status into something HW independent
      bStat = ((bDevStat & CON) ? DEV_STATUS_CONNECT : 0) |
              ((bDevStat & SUS) ? DEV_STATUS_SUSPEND : 0) |
              ((bDevStat & RST) ? DEV_STATUS_RESET : 0);
      // call handler
      //if (_pfnDevIntHandler != NULL) {
      //  DEBUG_LED_ON(8);    
      //  _pfnDevIntHandler(bStat);
      //  DEBUG_LED_OFF(8);   
      //}
      DEBUG_LED_ON(8);    
      usb_device_status_handler(bStat);
      DEBUG_LED_OFF(8);   
    }
  }
  
  // endpoint interrupt
  if (dwStatus & EP_SLOW) {
    USBDevIntClr = EP_SLOW; /* clear interrupt flag */
    for (i = 0; i < 32; i++) { /* check all endpoints */
      dwIntBit = (1 << i);
      if (USBEpIntSt & dwIntBit) {
        // clear int (and retrieve status)
        USBEpIntClr = dwIntBit;
        _usbWaitForDeviceIntFlags(CDFULL);
        bEPStat = USBCmdData;
        // convert EP pipe stat into something HW independent
        bStat = ((bEPStat & EPSTAT_FE)  ? EP_STATUS_DATA    : 0) |
                ((bEPStat & EPSTAT_ST)  ? EP_STATUS_STALLED : 0) |
                ((bEPStat & EPSTAT_STP) ? EP_STATUS_SETUP   : 0) |
                ((bEPStat & EPSTAT_EPN) ? EP_STATUS_NACKED  : 0) |
                ((bEPStat & EPSTAT_PO)  ? EP_STATUS_ERROR   : 0);
        // call handler
        if (usb_ep_handlers[i]) usb_ep_handlers[i](IDX2EP(i), bStat);
        //DBGNUM(i);
        //DBG("\n");
        //if (_apfnEPIntHandlers[i / 2] != NULL) {
        //  DEBUG_LED_ON(10);   
        //  _apfnEPIntHandlers[i / 2](IDX2EP(i), bStat);
        //  DEBUG_LED_OFF(10);
        //}
      }
    }
  }
  
  DEBUG_LED_OFF(9);   
  vicUpdatePriority();
}

/**
  Initialises the USB hardware
    
  This function assumes that the hardware is connected as shown in
  section 10.1 of the LPC2148 data sheet:
  * P0.31 controls a switch to connect a 1.5k pull-up to D+ if low.
  * P0.23 is connected to USB VCC.
  
  Embedded artists board: make sure to disconnect P0.23 LED as it
  acts as a pull-up and so prevents detection of USB disconnect.
    
  @return TRUE if the hardware was successfully initialised
 */
void usbInit(void)
{
  // configure P0.23 for Vbus sense
  PINSEL1 = (PINSEL1 & ~(3 << 14)) | (1 << 14); // P0.23
  // configure P0.31 for CONNECT
  PINSEL1 = (PINSEL1 & ~(3 << 30)) | (2 << 30); // P0.31
  
  PCONP |= (1 << 31);   /* enable USB peripheral */
  
  // initialise PLL, assuming a 12MHz crystal
  PLL48CON = 1;     // enable PLL
  PLL48CFG = (1 << 5) | 3; // P = 2, M = 4
  PLL48FEED = 0xAA;
  PLL48FEED = 0x55;
  while ((PLL48STAT & (1 << 10)) == 0);
  
  PLL48CON = 3;     // enable and connect
  PLL48FEED = 0xAA;
  PLL48FEED = 0x55;
  
  // disable/clear all interrupts for now
  USBDevIntEn  = 0;
  USBDevIntClr = 0xFFFFFFFF;
  USBDevIntPri = 0;
  
  USBEpIntEn   = 0;
  USBEpIntClr  = 0xFFFFFFFF;
  USBEpIntPri  = 0;
  
  // by default, only ACKs generate interrupts
  usbEnableNAKInterrupts(0);
  
  // init debug leds
  DEBUG_LED_INIT(8);
  DEBUG_LED_INIT(9);
  DEBUG_LED_INIT(10);
  
  DBG("usbInit 1\n");

  vicEnableIRQ(INT_CHANNEL_USB, 0 /* priority */, _usbIsr);

  _usbConfigureEP(0x00, MAX_PACKET_SIZE0);
  _usbConfigureEP(0x80, MAX_PACKET_SIZE0);

  DBG("usbInit 1\n");
}

/* usbstruct.h */


/*************************************************************************
    USB configuration
**************************************************************************/

//bool _usbHardwareInit         (void);
//void USBHwISR         (void);
//void usbEnableNAKInterrupts   (uint8_t bIntBits);
//void _usbConnect      (bool fConnect);
//void _usbSetAddress   (uint8_t bAddr);
//void usbConfigureDevice   (bool fConfigured);
//void _usbConfigureEP      (uint8_t bEP, uint16_t wMaxPacketSize);
//int  usbRead      (uint8_t bEP, uint8_t *pbBuf, int iMaxLen);
//int  usbWrite     (uint8_t bEP, uint8_t *pbBuf, int iLen);
//void _usbStallEP      (uint8_t bEP, bool fStall);
//uint8_t   USBHwEPGetStatus    (uint8_t bEP);

//void USBHwRegisterEPIntHandler    (uint8_t bEP, usb_ep_handler_t *pfnHandler);
//void USBHwRegisterDevIntHandler   (usb_dev_handler_t *pfnHandler);
//void USBHwRegisterFrameHandler(usb_frame_handler_t *pfnHandler);

/*************************************************************************
    USB application interface
**************************************************************************/


typedef bool (TFnHandleRequest)(int *piLen, uint8_t **ppbData);

static uint8_t  _usbControlTransferBuffer[8];
static int      _usbControlTransferRemainingLen;  /**< remaining bytes in buffer */

#ifndef usb_control_standard_custom_handler
#define usb_control_standard_custom_handler() 0
#endif

#ifndef usb_control_vendor_handler
#define usb_control_vendor_handler() 0
#endif

#ifndef usb_control_reserved_handler
#define usb_control_reserved_handler() 0
#endif

/** Array of installed request handler callbacks */
//static TFnHandleRequest *apfnReqHandlers[4] = {NULL, NULL, NULL, NULL};
/** Array of installed request data pointers */
//static uint8_t        *apbDataStore[4] = {NULL, NULL, NULL, NULL};

//bool USBInit(void);
//void USBRegisterRequestHandler(int iType, TFnHandleRequest *pfnHandler, uint8_t *pbDataStore);
//void USBRegisterCustomReqHandler(TFnHandleRequest *pfnHandler);
//typedef bool (TFnGetDescriptor)(uint16_t wTypeIndex, uint16_t wLangID);
//bool USBHandleStandardRequest(usb_setup_packet_t *pSetup);
//void USBHandleControlTransfer(uint8_t bEP, uint8_t bEPStat);
//void USBRegisterDescriptors(const uint8_t *pabDescriptors);
//bool usbGetDescriptor(uint16_t wTypeIndex, uint16_t wLangID);

/** @file
    Standard request handler.
    
    This modules handles the 'chapter 9' processing, specifically the
    standard device requests in table 9-3 from the universal serial bus
    specification revision 2.0
    
    Specific types of devices may specify additional requests (for example
    HID devices add a GET_DESCRIPTOR request for interfaces), but they
    will not be part of this module.

    @todo some requests have to return a request error if device not configured:
    @todo GET_INTERFACE, GET_STATUS, SET_INTERFACE, SYNCH_FRAME
    @todo this applies to the following if endpoint != 0:
    @todo SET_FEATURE, GET_FEATURE 
*/

//#include "type.h"
//#include "usbdebug.h"
//#include "usbapi.h"

//#define MAX_DESC_HANDLERS 4       /**< device, interface, endpoint, other */


/* general descriptor field offsets */
#define DESC_bLength                    0   /**< length offset */
#define DESC_bDescriptorType            1   /**< descriptor type offset */  

/* config descriptor field offsets */
#define CONF_DESC_wTotalLength          2   /**< total length offset */
#define CONF_DESC_bConfigurationValue   5   /**< configuration value offset */  
#define CONF_DESC_bmAttributes          7   /**< configuration characteristics */

/* interface descriptor field offsets */
#define INTF_DESC_bAlternateSetting     3   /**< alternate setting offset */

/* endpoint descriptor field offsets */
#define ENDP_DESC_bEndpointAddress      2   /**< endpoint address offset */
#define ENDP_DESC_wMaxPacketSize        4   /**< maximum packet size offset */


/** Currently selected configuration */
static uint8_t              bConfiguration = 0;
/** Installed custom request handler */
//static TFnHandleRequest   *pfnHandleCustomReq = NULL;
/** Pointer to registered descriptors */
//static const uint8_t          *pabDescrip = NULL;


/**
    Registers a pointer to a descriptor block containing all descriptors
    for the device.

    @param [in] pabDescriptors  The descriptor byte array
 */
//void USBRegisterDescriptors(const uint8_t *pabDescriptors)
//{
//  pabDescrip = pabDescriptors;
//}


/**
    Parses the list of installed USB descriptors and attempts to find
    the specified USB descriptor.
        
    @param [in]     wTypeIndex  Type and index of the descriptor
    @param [in]     wLangID     Language ID of the descriptor (currently unused)
    @param [out]    *piLen      Descriptor length
    @param [out]    *ppbData    Descriptor data
    
    @return TRUE if the descriptor was found, FALSE otherwise
 */
static bool usbGetDescriptor(uint16_t wTypeIndex, uint16_t wLangID) {
    uint8_t bType, bIndex;
    uint8_t *pab;
    int iCurIndex;
    
    ASSERT(usb_descriptors != NULL);
    
    bType = GET_DESC_TYPE(wTypeIndex);
    bIndex = GET_DESC_INDEX(wTypeIndex);
    
    pab = (uint8_t *)usb_descriptors;
    iCurIndex = 0;
    
    while (pab[DESC_bLength] != 0) {
        if (pab[DESC_bDescriptorType] == bType) {
            if (iCurIndex == bIndex) {
                // set data pointer
        usbControlTransferPtr = pab;
                // get length from structure
                if (bType == DESC_CONFIGURATION) {
                    // configuration descriptor is an exception, length is at offset 2 and 3
                    usbControlTransferLen = (pab[CONF_DESC_wTotalLength]) |
                                (pab[CONF_DESC_wTotalLength + 1] << 8);
                } else {
                    // normally length is at offset 0
                    usbControlTransferLen = pab[DESC_bLength];
                }
                return TRUE;
            }
            iCurIndex++;
        }
        // skip to next descriptor
        pab += pab[DESC_bLength];
    }
    // nothing found
    DBG("Desc type=");
  DBGNUM(bType);
  DBG(" index=");
  DBGNUM(bIndex);
    DBG(" not found!\n");
    return FALSE;
}


/**
    Configures the device according to the specified configuration index and
    alternate setting by parsing the installed USB descriptor list.
    A configuration index of 0 unconfigures the device.
        
    @param [in]     bConfigIndex    Configuration index
    @param [in]     bAltSetting     Alternate setting number
    
    @todo function always returns TRUE, add stricter checking?
    
    @return TRUE if successfully configured, FALSE otherwise
 */
static bool USBSetConfiguration(uint8_t bConfigIndex, uint8_t bAltSetting)
{
    uint8_t *pab;
    uint8_t bCurConfig, bCurAltSetting;
    uint8_t bEP;
    uint16_t    wMaxPktSize;
  
  DBG("set configuration\n");
    
    ASSERT(usb_descriptors != NULL);

    if (bConfigIndex == 0) {
        // unconfigure device
        usbUnconfigureDevice();
    }
    else {
        // configure endpoints for this configuration/altsetting
        pab = (uint8_t *)usb_descriptors;
        bCurConfig = 0xFF;
        bCurAltSetting = 0xFF;

        while (pab[DESC_bLength] != 0) {

            switch (pab[DESC_bDescriptorType]) {

            case DESC_CONFIGURATION:
                // remember current configuration index
                bCurConfig = pab[CONF_DESC_bConfigurationValue];
                break;

            case DESC_INTERFACE:
                // remember current alternate setting
                bCurAltSetting = pab[INTF_DESC_bAlternateSetting];
                break;

            case DESC_ENDPOINT:
                if ((bCurConfig == bConfigIndex) &&
                    (bCurAltSetting == bAltSetting)) {
                    // endpoint found for desired config and alternate setting
                    bEP = pab[ENDP_DESC_bEndpointAddress];
                    wMaxPktSize =   (pab[ENDP_DESC_wMaxPacketSize]) |
                                    (pab[ENDP_DESC_wMaxPacketSize + 1] << 8);
                    // configure endpoint
                    _usbConfigureEP(bEP, wMaxPktSize);
                }
                break;

            default:
                break;
            }
            // skip to next descriptor
            pab += pab[DESC_bLength];
        }
        
        // configure device
        usbConfigureDevice();
    }

    return TRUE;
}


/**
    Local function to handle a standard device request
        
    @param [in]     pSetup      The setup packet
    @param [in,out] *piLen      Pointer to data length
    @param [in,out] ppbData     Data buffer.

    @return TRUE if the request was handled successfully
 */
static bool _usbStdDeviceReqHandler(void) {
    switch (usbRequest.request) {
    
    case REQ_GET_STATUS:
        // bit 0: self-powered
        // bit 1: remote wakeup = not supported
        _usbControlTransferBuffer[0] = 0;
        _usbControlTransferBuffer[1] = 0;
    usbControlTransferPtr = _usbControlTransferBuffer;
        usbControlTransferLen = 2;
        break;
        
    case REQ_SET_ADDRESS:
        _usbSetAddress(usbRequest.data);
        break;

    case REQ_GET_DESCRIPTOR:
      DBG("D");
      DBGHEX(usbRequest.data,8);
      return usbGetDescriptor(usbRequest.data, usbRequest.index);

    case REQ_GET_CONFIGURATION:
        // indicate if we are configured
        _usbControlTransferBuffer[0] = bConfiguration;
    usbControlTransferPtr = _usbControlTransferBuffer;
        usbControlTransferLen = 1;
        break;

    case REQ_SET_CONFIGURATION:
        if (!USBSetConfiguration(usbRequest.data & 0xFF, 0)) {
            DBG("USBSetConfiguration failed!\n");
            return FALSE;
        }
        // configuration successful, update current configuration
        bConfiguration = usbRequest.data & 0xFF;    
        break;

    case REQ_CLEAR_FEATURE:
    case REQ_SET_FEATURE:
        if (usbRequest.data == FEA_REMOTE_WAKEUP) {
            // put DEVICE_REMOTE_WAKEUP code here
        }
        if (usbRequest.data == FEA_TEST_MODE) {
            // put TEST_MODE code here
        }
        return FALSE;

    case REQ_SET_DESCRIPTOR:
      DBG("Device req ");
      DBGNUM(usbRequest.request);
      DBG(" not implemented\n");
      return FALSE;

    default:
      DBG("Illegal device req ");
      DBGNUM(usbRequest.request);
      DBG("\n");
      return FALSE;
    }
    
    return TRUE;
}


/**
    Local function to handle a standard interface request
        
    @param [in]     pSetup      The setup packet
    @param [in,out] *piLen      Pointer to data length
    @param [in]     ppbData     Data buffer.

    @return TRUE if the request was handled successfully
 */
static bool _usbStdInterfaceReqHandler(void) {

    switch (usbRequest.request) {

    case REQ_GET_STATUS:
        // no bits specified
        _usbControlTransferBuffer[0] = 0;
        _usbControlTransferBuffer[1] = 0;
    usbControlTransferPtr = _usbControlTransferBuffer;
        usbControlTransferLen = 2;
        break;

    case REQ_CLEAR_FEATURE:
    case REQ_SET_FEATURE:
        // not defined for interface
        return FALSE;
    
    case REQ_GET_INTERFACE: // TODO use bNumInterfaces
        // there is only one interface, return n-1 (= 0)
        _usbControlTransferBuffer[0] = 0;
    usbControlTransferPtr = _usbControlTransferBuffer;
        usbControlTransferLen = 1;
        break;
    
    case REQ_SET_INTERFACE: // TODO use bNumInterfaces
        // there is only one interface (= 0)
        if (usbRequest.data != 0) {
            return FALSE;
        }
    usbControlTransferPtr = _usbControlTransferBuffer;
        usbControlTransferLen = 0;
        break;

    default:
      DBG("Illegal interface req ");
      DBGNUM(usbRequest.request);
      DBG("\n");
        return FALSE;
    }

    return TRUE;
}


/**
    Local function to handle a standard endpoint request
        
    @param [in]     pSetup      The setup packet
    @param [in,out] *piLen      Pointer to data length
    @param [in]     ppbData     Data buffer.

    @return TRUE if the request was handled successfully
 */
static bool _usbStdEPReqHandler(void) {

    switch (usbRequest.request) {
    case REQ_GET_STATUS:
        // bit 0 = endpointed halted or not
        _usbControlTransferBuffer[0] = (_usbGetEPStatus(usbRequest.index) & EP_STATUS_STALLED) ? 1 : 0;
        _usbControlTransferBuffer[1] = 0;
    usbControlTransferPtr = _usbControlTransferBuffer;
        usbControlTransferLen = 2;
        break;
        
    case REQ_CLEAR_FEATURE:
        if (usbRequest.data == FEA_ENDPOINT_HALT) {
            // clear HALT by unstalling
            _usbUnstallEP(usbRequest.index);
            break;
        }
        // only ENDPOINT_HALT defined for endpoints
        return FALSE;
    
    case REQ_SET_FEATURE:
        if (usbRequest.data == FEA_ENDPOINT_HALT) {
            // set HALT by stalling
            _usbStallEP(usbRequest.index);
            break;
        }
        // only ENDPOINT_HALT defined for endpoints
        return FALSE;

    case REQ_SYNCH_FRAME:
      DBG("Endpoint req ");
      DBGNUM(usbRequest.request);
      DBG(" not implemented\n");
      return FALSE;

    default:
      DBG("Illegal endpoint req ");
      DBGNUM(usbRequest.request);
      DBG("\n");
      return FALSE;
    }
    
    return TRUE;
}


/**
    Default handler for standard ('chapter 9') requests
    
    If a custom request handler was installed, this handler is called first.
        
    @param [in]     pSetup      The setup packet
    @param [in,out] *piLen      Pointer to data length
    @param [in]     ppbData     Data buffer.

    @return TRUE if the request was handled successfully
 */

bool _usbStandardRequestHandler(void) {
  
  if (usb_control_standard_custom_handler()) return TRUE;
    // try the custom request handler first
//  if ((pfnHandleCustomReq != NULL) && pfnHandleCustomReq(piLen, ppbData)) {
//      return TRUE;
//  }
    
    switch (REQTYPE_GET_RECIP(usbRequest.type)) {
    case REQTYPE_RECIP_DEVICE:      return _usbStdDeviceReqHandler();
    case REQTYPE_RECIP_INTERFACE:   return _usbStdInterfaceReqHandler();
    case REQTYPE_RECIP_ENDPOINT:    return _usbStdEPReqHandler();
    default: return FALSE;
    }
}


static bool _usbControlTransferDispatcher(void) {
  int t;
  
  t = REQTYPE_GET_TYPE(usbRequest.type);
  switch (t) {
    case 0: return _usbStandardRequestHandler();
    case 1: return usb_control_class_handler();
    case 2: return usb_control_vendor_handler();
    case 3: return usb_control_reserved_handler();
  }
}

static void _usbControlTransferStall(uint8_t bEPStat)
{
    _usbStallEP(0x80);

  {
    uint8_t *pb;
    int i;
    DBG("STALL on [");
      pb = (uint8_t *)&usbRequest;
      for (i = 0; i < 8; i++) {
        DBG(" ");
        DBGHEX(*pb++,2);
      }
      DBG("] stat=");
      DBGHEX(bEPStat,2);
      DBG("\n");
  }
}


/**
    Sends next chunk of data (possibly 0 bytes) to host
 */
static void _usbControlWrite(void)
{
    int l;

    l = MIN(MAX_PACKET_SIZE0, _usbControlTransferRemainingLen);
    usbWrite(0x80, usbControlTransferPtr, l);
    usbControlTransferPtr           += l;
    _usbControlTransferRemainingLen -= l;
}

static void usbEP0OutHandler(uint8_t bEP, uint8_t bEPStat) {
  int iChunk, iType;
  
  if (bEPStat & EP_STATUS_SETUP) {
    usbRead(0x00, (uint8_t *)&usbRequest, sizeof(usbRequest));
    //DBG("S");
    //DBGHEX(usbRequest.request,2);
    //DBG("\n");
      
    _usbControlTransferRemainingLen = usbRequest.length; /* for sending data out */
      
    if ((usbRequest.length == 0)
        || (REQTYPE_GET_DIR(usbRequest.type) == REQTYPE_DIR_TO_HOST)) {
      if (!_usbControlTransferDispatcher()) {
        DBG("_HandleRequest1 failed\n");
        _usbControlTransferStall(bEPStat);
        return;
      }
      _usbControlTransferRemainingLen = MIN(usbControlTransferLen, usbRequest.length);
      _usbControlWrite();
    }
  } else { /* not a setup phase */
    if (_usbControlTransferRemainingLen > 0) {
      // store data
      usbControlTransferPtr = _usbControlTransferBuffer;
      iChunk = usbRead(0x00, usbControlTransferPtr, _usbControlTransferRemainingLen);
      if (iChunk < 0) {
        _usbControlTransferStall(bEPStat);
        return;
      }
      usbControlTransferPtr += iChunk;
      _usbControlTransferRemainingLen -= iChunk;
      if (_usbControlTransferRemainingLen == 0) {
        // received all, send data to handler
        usbControlTransferPtr = _usbControlTransferBuffer;
        if (!_usbControlTransferDispatcher()) {
          DBG("_HandleRequest2 failed\n");
          _usbControlTransferStall(bEPStat);
          return;
        }
        // send status to host
        _usbControlWrite();
      }
    } else {
      // absorb zero-length status message
      iChunk = usbRead(0x00, NULL, 0);
      DBG(iChunk > 0 ? "?\n" : "-\n");
    }
  }
}

static void usbEP0InHandler(uint8_t bEP, uint8_t bEPStat) {
  _usbControlWrite();
}

#if 0
static void usbInit(void) {
  // init hardware
  _usbHardwareInit();
  
  // register bus reset handler
  //USBHwRegisterDevIntHandler(HandleUsbReset);
  
  //USBHwRegisterEPIntHandler(0x00, _usbControlTransferHandler);
  //USBHwRegisterEPIntHandler(0x80, _usbControlTransferHandler);
  
  // setup control endpoints
  _usbConfigureEP(0x00, MAX_PACKET_SIZE0);
  _usbConfigureEP(0x80, MAX_PACKET_SIZE0);
  
  // register standard request handler
  //USBRegisterRequestHandler(REQTYPE_TYPE_STANDARD, USBHandleStandardRequest, abStdReqData);
}
#endif 
