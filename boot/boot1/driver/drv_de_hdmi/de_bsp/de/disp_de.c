#include "disp_de.h"
#include "disp_display.h"
#include "disp_event.h"
#include "disp_scaler.h"
#include "disp_clk.h"
#include "disp_lcd.h"

__s32 Image_init(__u32 sel)
{
    
    image_clk_init(sel);
	image_clk_on(sel);	//when access image registers, must open MODULE CLOCK of image
	DE_BE_Reg_Init(sel);
	
    BSP_disp_sprite_init(sel);
    BSP_disp_set_output_csc(sel, DISP_OUT_CSC_TYPE_LCD);
    
    Image_open(sel);

    DE_BE_EnableINT(sel, DE_IMG_REG_LOAD_FINISH);
    DE_BE_reg_auto_load_en(sel, 0);
	
    return DIS_SUCCESS;
}
      
__s32 Image_exit(__u32 sel)
{    
    DE_BE_DisableINT(sel, DE_IMG_REG_LOAD_FINISH);
    BSP_disp_sprite_exit(sel);
    image_clk_exit(sel);
        
    return DIS_SUCCESS;
}

__s32 Image_open(__u32  sel)
{
   DE_BE_Enable(sel);
      
   return DIS_SUCCESS;
}
      

__s32 Image_close(__u32 sel)
{
   DE_BE_Disable(sel);
   
   gdisp.screen[sel].status &= IMAGE_USED_MASK;
   
   return DIS_SUCCESS;
}


__s32 BSP_disp_set_bright(__u32 sel, __u32 bright)
{
    gdisp.screen[sel].bright = bright;
    DE_BE_Set_Enhance(sel, gdisp.screen[sel].bright, gdisp.screen[sel].contrast, gdisp.screen[sel].saturation, gdisp.screen[sel].hue);

    return DIS_SUCCESS;
}

__s32 BSP_disp_get_bright(__u32 sel)
{
    return gdisp.screen[sel].bright;
}

__s32 BSP_disp_set_contrast(__u32 sel, __u32 contrast)
{
    gdisp.screen[sel].contrast = contrast;
    DE_BE_Set_Enhance(sel, gdisp.screen[sel].bright, gdisp.screen[sel].contrast, gdisp.screen[sel].saturation, gdisp.screen[sel].hue);

    return DIS_SUCCESS;
}

__s32 BSP_disp_get_contrast(__u32 sel)
{
    return gdisp.screen[sel].contrast;
}

__s32 BSP_disp_set_saturation(__u32 sel, __u32 saturation)
{
    gdisp.screen[sel].saturation = saturation;
    DE_BE_Set_Enhance(sel, gdisp.screen[sel].bright, gdisp.screen[sel].contrast, gdisp.screen[sel].saturation, gdisp.screen[sel].hue);

    return DIS_SUCCESS;
}

__s32 BSP_disp_get_saturation(__u32 sel)
{
    return gdisp.screen[sel].saturation;
}

__s32 BSP_disp_set_hue(__u32 sel, __u32 hue)
{
    gdisp.screen[sel].hue = hue;
    DE_BE_Set_Enhance(sel, gdisp.screen[sel].bright, gdisp.screen[sel].contrast, gdisp.screen[sel].saturation, gdisp.screen[sel].hue);

    return DIS_SUCCESS;
}

__s32 BSP_disp_get_hue(__u32 sel)
{
    return gdisp.screen[sel].hue;
}

__s32 BSP_disp_enhance_enable(__u32 sel, __bool enable)
{
    DE_BE_enhance_enable(sel, enable);
    gdisp.screen[sel].enhance_en = enable;

    return DIS_SUCCESS;
}

__s32 BSP_disp_get_enhance_enable(__u32 sel)
{
    return gdisp.screen[sel].enhance_en;
}


__s32 BSP_disp_set_screen_size(__u32 sel, __disp_rectsz_t * size)
{    
    DE_BE_set_display_size(sel, size->width, size->height);

    gdisp.screen[sel].screen_width = size->width;
    gdisp.screen[sel].screen_height= size->height;

    return DIS_SUCCESS;
}

__s32 BSP_disp_set_output_csc(__u32 sel, __disp_out_csc_type_t type)
{
    __disp_color_range_t out_color_range = DISP_COLOR_RANGE_0_255;
    __u32 out_csc = 0;



    if(type == DISP_OUT_CSC_TYPE_HDMI_YUV)
    {
        __s32 ret = 0;
        __s32 value = 0;
        
        out_color_range = DISP_COLOR_RANGE_16_255;

        ret = OSAL_Script_FetchParser_Data("disp_init", "screen0_out_color_range", &value, 1);
        if(ret < 0)
        {
            DE_INF("fetch script data disp_init.screen0_out_color_range fail\n");
        }
        else
        {
            out_color_range = value;
            DE_INF("screen0_out_color_range = %d\n", value);
        }
        out_csc = 2;
    }else if(type == DISP_OUT_CSC_TYPE_HDMI_RGB)
    {
        __s32 ret = 0;
        __s32 value = 0;
        
        out_color_range = DISP_COLOR_RANGE_16_255;

        ret = OSAL_Script_FetchParser_Data("disp_init", "screen0_out_color_range", &value, 1);
        if(ret < 0)
        {
            DE_INF("fetch script data disp_init.screen0_out_color_range fail\n");
        }
        else
        {
            out_color_range = value;
            DE_INF("screen0_out_color_range = %d\n", value);
        }
        out_csc = 0;
    }
    else if(type == DISP_OUT_CSC_TYPE_TV)
    {
        out_csc = 1;
    }
   
    DE_BE_Output_Cfg_Csc_Coeff(sel, out_csc, out_color_range);

    return DIS_SUCCESS;
}

__s32 BSP_disp_de_flicker_enable(__u32 sel, __bool b_en)
{   
	if(b_en)
	{
		gdisp.screen[sel].de_flicker_status |= DE_FLICKER_REQUIRED;
	}
	else
	{
		gdisp.screen[sel].de_flicker_status &= DE_FLICKER_REQUIRED_MASK;
	}
	Disp_set_out_interlace(sel);
	return DIS_SUCCESS;
}

__s32 Disp_set_out_interlace(__u32 sel)
{
	__u32 i;
	__bool b_cvbs_out = 0;

	if(gdisp.screen[sel].output_type==DISP_OUTPUT_TYPE_TV && 
	    (gdisp.screen[sel].tv_mode==DISP_TV_MOD_PAL || gdisp.screen[sel].tv_mode==DISP_TV_MOD_PAL_M ||
	    gdisp.screen[sel].tv_mode==DISP_TV_MOD_PAL_NC || gdisp.screen[sel].tv_mode==DISP_TV_MOD_NTSC))
	{
	    b_cvbs_out = 1;
	}

    gdisp.screen[sel].de_flicker_status |= DE_FLICKER_REQUIRED;

    BSP_disp_cfg_start(sel);

	if((gdisp.screen[sel].de_flicker_status & DE_FLICKER_REQUIRED) && b_cvbs_out)	//when output device is cvbs
	{
		DE_BE_deflicker_enable(sel, TRUE);
        for(i=0; i<2; i++)
        {
            if((gdisp.scaler[i].status & SCALER_USED) && (gdisp.scaler[i].screen_index == sel))
            {
				Scaler_Set_Outitl(i, FALSE);
				gdisp.scaler[i].b_reg_change = TRUE;
			}
		}
		gdisp.screen[sel].de_flicker_status |= DE_FLICKER_USED;
	}
	else
	{
	    DE_BE_deflicker_enable(sel, FALSE);
        for(i=0; i<2; i++)
        {
            if((gdisp.scaler[i].status & SCALER_USED) && (gdisp.scaler[i].screen_index == sel))
    		{
    			Scaler_Set_Outitl(i, gdisp.screen[sel].b_out_interlace);
    			gdisp.scaler[i].b_reg_change = TRUE;
    		}
    	}
    	gdisp.screen[sel].de_flicker_status &= DE_FLICKER_USED_MASK;
    }
	DE_BE_Set_Outitl_enable(sel, gdisp.screen[sel].b_out_interlace);

    BSP_disp_cfg_finish(sel);
	return DIS_SUCCESS;
}
