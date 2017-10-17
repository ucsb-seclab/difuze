//
// Created by machiry on 1/30/17.
//

#include <iostream>
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/IR/Module.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#define SIZEOF_ARRAY(a) (sizeof(a) / sizeof(a[0]))

using namespace std;
using namespace llvm;
#define DR_LINK_OUT "llvm_link_out"
class BitCodeFunctions {
public:
    std::string target_bc_file;
    std::vector<string> definedFunctions;
    std::vector<string> declaredFunctions;
    BitCodeFunctions(const char *curr_file, std::vector<string> &defined_functions,
                     std::vector<string> &declared_functions) {
        target_bc_file = curr_file;
        definedFunctions.clear();
        definedFunctions.insert(definedFunctions.end(), defined_functions.begin(), defined_functions.end());
        declaredFunctions.clear();
        declaredFunctions.insert(declaredFunctions.end(), declared_functions.begin(), declared_functions.end());

	// Sort definedFunctions vector to accelerate search
        std::sort(definedFunctions.begin(), definedFunctions.end());
    }

    bool has_function(const char *to_check_function) {
        std::string curr_func = to_check_function;
        return has_function(curr_func);
    }
    bool has_function(std::string &to_check_function) {
        std::vector<string>::iterator low;

        low = std::lower_bound(definedFunctions.begin(), definedFunctions.end(), to_check_function);
        if ((low != definedFunctions.end()) &&
            ((*low).compare(to_check_function) == 0)) {
            return true;
        }

        return false;
    }

    bool isNeeded(BitCodeFunctions *dstBCFile) {
        for(std::string &currDecFunction:declaredFunctions) {
            if(dstBCFile->has_function(currDecFunction)) {
                return true;
            }
        }
        return false;
    }


};
// folders
// mediatek folders
static const char *mediatek_driver_folder[] = {"drivers/misc/mediatek",
                                       "sound/soc/mediatek"};
// huawei folders
static const char *huawei_driver_folders[] = {"drivers/connectivity/hisi",
                                      "drivers/contexthub",
                                      "drivers/devfreq/hisi",
                                      "drivers/dsm",
                                      "drivers/gpio/gpio-hi6402.c",
                                      "drivers/gpu/ion/hisi",
                                      "drivers/gpu/arm/r8p0-01dev0/platform/hisilicon",
                                      "drivers/gpu/arm/r5p0-06rel0/platform/hi3635",
                                      "drivers/gpu/arm/r5p0-06rel0/platform/hi3650",
                                      "drivers/gpu/arm/r6p0-01dev0/platform/hi6250",
                                      "drivers/gpu/arm/r7p0-02rel0/platform/hisilicon",
                                      "drivers/hisi",
                                      "drivers/hisi_pilot",
                                      "drivers/huawei_platform",
                                      "drivers/hwspinlock",
                                      "drivers/input/keyboard/hisi",
                                      "drivers/input/misc/hisi",
                                      "drivers/iommu/hisi",
                                      "drivers/leds/hisi",
                                      "drivers/media/huawei",
                                      "drivers/media/platform/ovisp23",
                                      "drivers/mfd/hisi",
                                      "drivers/misc/hilog",
                                      "drivers/mmc/card",
                                      "drivers/mmc/host",
                                      "drivers/mtd/hisi",
                                      "drivers/rpmsg",
                                      "drivers/rtc",
                                      "drivers/srecorder",
                                      "drivers/staging/android/ion/hisi",
                                      "drivers/vcodec/hi_vcodec",
                                      "drivers/vendor/hisi",
                                      "drivers/video/hisi",
                                      "drivers/watchdog",
                                      "sound/soc/hisilicon"};
// qualcomm folders
static const char *qualcomm_driver_folders[] = {"drivers/char/msm",
                                        "drivers/char/rdbg",
                                        "drivers/char/tpm",
                                        "drivers/char/hw_random/msm",
                                        "drivers/char/diag",
                                        "drivers/char/adsprpc",
                                        "drivers/crypto/msm",
                                        "drivers/gpio",
                                        "drivers/gpu/msm",
                                        "drivers/gpu/ion",
                                        "drivers/gud",
                                        "drivers/hwmon/msm",
                                        "drivers/input/touchscreen/msm",
                                        "drivers/iommu/msm",
                                        "drivers/media/radio/radio-iris",
                                        "drivers/media/platform/msm",
                                        "drivers/misc/qseecom",
                                        "drivers/mmc/host/msm",
                                        "drivers/net/ethernet/msm",
                                        "drivers/platform/msm",
                                        "drivers/spi",
                                        "drivers/thermal/msm",
                                        "drivers/tty/serial/msm",
                                        "drivers/uio/msm",
                                        "drivers/video/msm",
                                        "sound/soc/msm",
                                        "arch/arm/mach-msm",
                                        "arch/arm64/mach-msm"};
static const char *samsung_driver_folders[] = {"drivers/acpi/video_detect.c",
                                              "drivers/acpi/ec.c",
                                              "drivers/acpi/video.c",
                                              "drivers/ata/pata_samsung_cf.c",
                                              "drivers/ata/ahci_da850.c",
                                              "drivers/ata/pata_pcmcia.c",
                                              "drivers/ata/ata_piix.c",
                                              "drivers/ata/libata-core.c",
                                              "drivers/ata/ahci.c",
                                              "drivers/base/dma-contiguous.c",
                                              "drivers/base/power/domain.c",
                                              "drivers/base/core.c",
                                              "drivers/battery/max77804_charger.c",
                                              "drivers/battery/da9155_charger.c",
                                              "drivers/battery/bq51221_charger.c",
                                              "drivers/battery/sec_fuelgauge.c",
                                              "drivers/battery/max77854_fuelgauge.c",
                                              "drivers/battery/s2mu003_charger.c",
                                              "drivers/battery/smb347_charger.c",
                                              "drivers/battery/sec_battery_data.c",
                                              "drivers/battery/sec_battery.c",
                                              "drivers/battery/p9220_charger.c",
                                              "drivers/battery/sec_charger.c",
                                              "drivers/battery/sec_multi_charger.c",
                                              "drivers/battery/max17048_fuelgauge.c",
                                              "drivers/battery/max77823_charger.c",
                                              "drivers/battery/max77843_charger.c",
                                              "drivers/battery/max17050_fuelgauge.c",
                                              "drivers/battery/max77833_fuelgauge.c",
                                              "drivers/battery/max77833_charger.c",
                                              "drivers/battery/max77854_charger.c",
                                              "drivers/battery/sec_adc.c",
                                              "drivers/battery/max77823_fuelgauge.c",
                                              "drivers/battery/max77843_fuelgauge.c",
                                              "drivers/battery/bq24260_charger.c",
                                              "drivers/battery/sec_cisd.c",
                                              "drivers/battery/sec_step_charging.c",
                                              "drivers/battery/max77693_charger.c",
                                              "drivers/battery_v2/da9155_charger.c",
                                              "drivers/battery_v2/max77854_fuelgauge.c",
                                              "drivers/battery_v2/sec_battery_data.c",
                                              "drivers/battery_v2/sec_battery.c",
                                              "drivers/battery_v2/p9220_charger.c",
                                              "drivers/battery_v2/sec_multi_charger.c",
                                              "drivers/battery_v2/max77854_charger.c",
                                              "drivers/battery_v2/sec_adc.c",
                                              "drivers/battery_v2/sec_step_charging.c",
                                              "drivers/bluetooth/bcm4359.c",
                                              "drivers/bluetooth/bcm4358.c",
                                              "drivers/bts/cal_bts8890.c",
                                              "drivers/bts/bts-exynos8890.c",
                                              "drivers/bts/cal_bts.c",
                                              "drivers/ccic/s2mm005.c",
                                              "drivers/ccic/s2mm003_fw.c",
                                              "drivers/ccic/ccic_sysfs.c",
                                              "drivers/ccic/ccic_alternate.c",
                                              "drivers/ccic/s2mm005_cc.c",
                                              "drivers/ccic/s2mm003.c",
                                              "drivers/ccic/s2mm005_pd.c",
                                              "drivers/char/exynos-mcomp/mcomp.c",
                                              "drivers/char/mst_ctrl.c",
                                              "drivers/char/hw_random/exyswd-rng.c",
                                              "drivers/char/hw_random/exynos-rng.c",
                                              "drivers/clk/clk-max-gen.c",
                                              "drivers/clk/rockchip/clk.c",
                                              "drivers/clk/rockchip/clk-cpu.c",
                                              "drivers/clk/clk-s2mps11.c",
                                              "drivers/clk/samsung/clk-s3c64xx.c",
                                              "drivers/clk/samsung/clk-exynos5440.c",
                                              "drivers/clk/samsung/clk-exynos5420.c",
                                              "drivers/clk/samsung/clk-exynos5250.c",
                                              "drivers/clk/samsung/clk-exynos-pwm.c",
                                              "drivers/clk/samsung/clk-s5pv210-audss.c",
                                              "drivers/clk/samsung/clk-exynos8890.c",
                                              "drivers/clk/samsung/clk.c",
                                              "drivers/clk/samsung/clk-exynos-audss.c",
                                              "drivers/clk/samsung/clk-s5pv210.c",
                                              "drivers/clk/samsung/clk-exynos-clkout.c",
                                              "drivers/clk/samsung/clk-s3c2443.c",
                                              "drivers/clk/samsung/clk-exynos5260.c",
                                              "drivers/clk/samsung/clk-exynos5410.c",
                                              "drivers/clk/samsung/clk-pll.c",
                                              "drivers/clk/samsung/clk-exynos3250.c",
                                              "drivers/clk/samsung/clk-exynos4.c",
                                              "drivers/clk/samsung/clk-s3c2412.c",
                                              "drivers/clk/samsung/composite.c",
                                              "drivers/clk/samsung/clk-s3c2410.c",
                                              "drivers/clk/clk-conf.c",
                                              "drivers/clk/clk-max77802.c",
                                              "drivers/clk/clk-max77686.c",
                                              "drivers/clocksource/samsung_pwm_timer.c",
                                              "drivers/clocksource/exynos_mct.c",
                                              "drivers/cpufreq/cpufreq_interactive.c",
                                              "drivers/cpufreq/s5pv210-cpufreq.c",
                                              "drivers/cpufreq/exynos4x12-cpufreq.c",
                                              "drivers/cpufreq/exynos-mp-cpufreq.c",
                                              "drivers/cpufreq/exynos4210-cpufreq.c",
                                              "drivers/cpufreq/sa1110-cpufreq.c",
                                              "drivers/cpufreq/exynos-cpufreq.c",
                                              "drivers/cpufreq/exynos5250-cpufreq.c",
                                              "drivers/cpufreq/exynos-mp-cpufreq-cal.c",
                                              "drivers/cpufreq/exynos5440-cpufreq.c",
                                              "drivers/cpuidle/cpuidle_profiler.c",
                                              "drivers/cpuidle/cpuidle-exynos.c",
                                              "drivers/cpuidle/cpuidle-big_little.c",
                                              "drivers/cpuidle/cpuidle-exynos64.c",
                                              "drivers/crypto/s5p-sss.c",
                                              "drivers/crypto/fmp/fmplib.c",
                                              "drivers/crypto/fmp/first_file.c",
                                              "drivers/crypto/fmp/last_file.c",
                                              "drivers/crypto/fmp/fmp_ufs.c",
                                              "drivers/crypto/fmp/fmp_integrity.c",
                                              "drivers/crypto/fmp/fmp_ufs_fips.c",
                                              "drivers/crypto/fmp/fmpdev.c",
                                              "drivers/crypto/fmp/fmp_derive_iv.c",
                                              "drivers/devfreq/devfreq.c",
                                              "drivers/devfreq/governor_simpleusage.c",
                                              "drivers/devfreq/governor_simpleondemand.c",
                                              "drivers/devfreq/governor_performance.c",
                                              "drivers/devfreq/governor_simpleexynos.c",
                                              "drivers/devfreq/governor_powersave.c",
                                              "drivers/devfreq/exynos/exynos_ppmu.c",
                                              "drivers/devfreq/exynos/exynos8890_bus_disp.c",
                                              "drivers/devfreq/exynos/exynos8890_bus_mif.c",
                                              "drivers/devfreq/exynos/exynos5_bus.c",
                                              "drivers/devfreq/exynos/exynos4_bus.c",
                                              "drivers/devfreq/exynos/exynos-devfreq.c",
                                              "drivers/devfreq/exynos/exynos8890_bus_int.c",
                                              "drivers/devfreq/exynos/exynos8890_bus_cam.c",
                                              "drivers/devfreq/governor_userspace.c",
                                              "drivers/dma/amba-pl08x.c",
                                              "drivers/dma/samsung-dma.c",
                                              "drivers/dma/pl330.c",
                                              "drivers/extcon/extcon-max8997.c",
                                              "drivers/extcon/extcon-sm5502.c",
                                              "drivers/extcon/extcon-gpio.c",
                                              "drivers/extcon/extcon-class.c",
                                              "drivers/extcon/extcon-max77693.c",
                                              "drivers/extcon/extcon-rt8973a.c",
                                              "drivers/extcon/extcon-max14577.c",
                                              "drivers/extcon/extcon-adc-jack.c",
                                              "drivers/fingerprint/vfs7xxx.c",
                                              "drivers/fingerprint/fingerprint_sysfs.c",
                                              "drivers/fingerprint/et320-spi_data_transfer.c",
                                              "drivers/fingerprint/et320-spi.c",
                                              "drivers/gpio/gpio-samsung.c",
                                              "drivers/gps/sec_gps_bcm4753.c",
                                              "drivers/gps/sec_gps_bcm47531.c",
                                              "drivers/gpu/arm/t8xx/r9p0/platform/exynos/gpu_hwcnt.c",
                                              "drivers/gpu/arm/t8xx/r9p0/platform/exynos/mali_kbase_platform.c",
                                              "drivers/gpu/arm/t8xx/r9p0/platform/exynos/gpu_custom_interface.c",
                                              "drivers/gpu/arm/t8xx/r9p0/platform/exynos/gpu_control.c",
                                              "drivers/gpu/arm/t8xx/r9p0/platform/exynos/gpu_notifier.c",
                                              "drivers/gpu/arm/t8xx/r9p0/platform/exynos/gpu_exynos8890.c",
                                              "drivers/gpu/arm/t8xx/r9p0/platform/exynos/gpu_perf.c",
                                              "drivers/gpu/arm/t8xx/r9p0/platform/exynos/gpu_utilization.c",
                                              "drivers/gpu/arm/t8xx/r9p0/platform/exynos/gpu_balance.c",
                                              "drivers/gpu/arm/t8xx/r9p0/platform/exynos/gpu_dvfs_api.c",
                                              "drivers/gpu/arm/t8xx/r9p0/platform/exynos/gpu_dvfs_handler.c",
                                              "drivers/gpu/arm/t8xx/r9p0/platform/exynos/gpu_dvfs_governor.c",
                                              "drivers/gpu/arm/t8xx/r9p0/platform/exynos/gpu_ipa.c",
                                              "drivers/gpu/arm/t8xx/r9p0/platform/exynos/gpu_hwcnt_sec.c",
                                              "drivers/gpu/arm/t8xx/r9p0/platform/exynos/gpu_pmqos.c",
                                              "drivers/gpu/arm/t8xx/r9p0/platform/exynos/gpu_exynos7890.c",
                                              "drivers/gpu/arm/t8xx/r9p0/platform/exynos/gpu_integration_callbacks.c",
                                              "drivers/gpu/arm/t8xx/r7p0/platform/exynos/gpu_hwcnt.c",
                                              "drivers/gpu/arm/t8xx/r7p0/platform/exynos/mali_kbase_platform.c",
                                              "drivers/gpu/arm/t8xx/r7p0/platform/exynos/gpu_custom_interface.c",
                                              "drivers/gpu/arm/t8xx/r7p0/platform/exynos/gpu_control.c",
                                              "drivers/gpu/arm/t8xx/r7p0/platform/exynos/gpu_notifier.c",
                                              "drivers/gpu/arm/t8xx/r7p0/platform/exynos/gpu_exynos8890.c",
                                              "drivers/gpu/arm/t8xx/r7p0/platform/exynos/gpu_perf.c",
                                              "drivers/gpu/arm/t8xx/r7p0/platform/exynos/gpu_utilization.c",
                                              "drivers/gpu/arm/t8xx/r7p0/platform/exynos/gpu_balance.c",
                                              "drivers/gpu/arm/t8xx/r7p0/platform/exynos/gpu_dvfs_api.c",
                                              "drivers/gpu/arm/t8xx/r7p0/platform/exynos/gpu_dvfs_handler.c",
                                              "drivers/gpu/arm/t8xx/r7p0/platform/exynos/gpu_dvfs_governor.c",
                                              "drivers/gpu/arm/t8xx/r7p0/platform/exynos/gpu_ipa.c",
                                              "drivers/gpu/arm/t8xx/r7p0/platform/exynos/gpu_hwcnt_sec.c",
                                              "drivers/gpu/arm/t8xx/r7p0/platform/exynos/gpu_pmqos.c",
                                              "drivers/gpu/arm/t8xx/r7p0/platform/exynos/gpu_exynos7890.c",
                                              "drivers/gpu/arm/t8xx/r7p0/platform/exynos/gpu_integration_callbacks.c",
                                              "drivers/gpu/arm/t8xx/r12p0/platform/exynos/gpu_hwcnt.c",
                                              "drivers/gpu/arm/t8xx/r12p0/platform/exynos/mali_kbase_platform.c",
                                              "drivers/gpu/arm/t8xx/r12p0/platform/exynos/gpu_custom_interface.c",
                                              "drivers/gpu/arm/t8xx/r12p0/platform/exynos/gpu_control.c",
                                              "drivers/gpu/arm/t8xx/r12p0/platform/exynos/gpu_notifier.c",
                                              "drivers/gpu/arm/t8xx/r12p0/platform/exynos/gpu_exynos8890.c",
                                              "drivers/gpu/arm/t8xx/r12p0/platform/exynos/gpu_perf.c",
                                              "drivers/gpu/arm/t8xx/r12p0/platform/exynos/gpu_utilization.c",
                                              "drivers/gpu/arm/t8xx/r12p0/platform/exynos/gpu_balance.c",
                                              "drivers/gpu/arm/t8xx/r12p0/platform/exynos/gpu_dvfs_api.c",
                                              "drivers/gpu/arm/t8xx/r12p0/platform/exynos/gpu_dvfs_handler.c",
                                              "drivers/gpu/arm/t8xx/r12p0/platform/exynos/gpu_dvfs_governor.c",
                                              "drivers/gpu/arm/t8xx/r12p0/platform/exynos/gpu_ipa.c",
                                              "drivers/gpu/arm/t8xx/r12p0/platform/exynos/gpu_hwcnt_sec.c",
                                              "drivers/gpu/arm/t8xx/r12p0/platform/exynos/gpu_pmqos.c",
                                              "drivers/gpu/arm/t8xx/r12p0/platform/exynos/gpu_exynos7890.c",
                                              "drivers/gpu/arm/t8xx/r12p0/platform/exynos/gpu_integration_callbacks.c",
                                              "drivers/gpu/arm/t8xx/r12p0_n/platform/exynos/gpu_hwcnt.c",
                                              "drivers/gpu/arm/t8xx/r12p0_n/platform/exynos/mali_kbase_platform.c",
                                              "drivers/gpu/arm/t8xx/r12p0_n/platform/exynos/gpu_custom_interface.c",
                                              "drivers/gpu/arm/t8xx/r12p0_n/platform/exynos/gpu_control.c",
                                              "drivers/gpu/arm/t8xx/r12p0_n/platform/exynos/gpu_notifier.c",
                                              "drivers/gpu/arm/t8xx/r12p0_n/platform/exynos/gpu_exynos8890.c",
                                              "drivers/gpu/arm/t8xx/r12p0_n/platform/exynos/gpu_perf.c",
                                              "drivers/gpu/arm/t8xx/r12p0_n/platform/exynos/gpu_utilization.c",
                                              "drivers/gpu/arm/t8xx/r12p0_n/platform/exynos/gpu_balance.c",
                                              "drivers/gpu/arm/t8xx/r12p0_n/platform/exynos/gpu_dvfs_api.c",
                                              "drivers/gpu/arm/t8xx/r12p0_n/platform/exynos/gpu_dvfs_handler.c",
                                              "drivers/gpu/arm/t8xx/r12p0_n/platform/exynos/gpu_dvfs_governor.c",
                                              "drivers/gpu/arm/t8xx/r12p0_n/platform/exynos/gpu_ipa.c",
                                              "drivers/gpu/arm/t8xx/r12p0_n/platform/exynos/gpu_hwcnt_sec.c",
                                              "drivers/gpu/arm/t8xx/r12p0_n/platform/exynos/gpu_pmqos.c",
                                              "drivers/gpu/arm/t8xx/r12p0_n/platform/exynos/gpu_exynos7890.c",
                                              "drivers/gpu/arm/t8xx/r12p0_n/platform/exynos/gpu_integration_callbacks.c",
                                              "drivers/gpu/arm/t8xx/r12p0_m/platform/exynos/gpu_hwcnt.c",
                                              "drivers/gpu/arm/t8xx/r12p0_m/platform/exynos/mali_kbase_platform.c",
                                              "drivers/gpu/arm/t8xx/r12p0_m/platform/exynos/gpu_custom_interface.c",
                                              "drivers/gpu/arm/t8xx/r12p0_m/platform/exynos/gpu_control.c",
                                              "drivers/gpu/arm/t8xx/r12p0_m/platform/exynos/gpu_notifier.c",
                                              "drivers/gpu/arm/t8xx/r12p0_m/platform/exynos/gpu_exynos8890.c",
                                              "drivers/gpu/arm/t8xx/r12p0_m/platform/exynos/gpu_perf.c",
                                              "drivers/gpu/arm/t8xx/r12p0_m/platform/exynos/gpu_utilization.c",
                                              "drivers/gpu/arm/t8xx/r12p0_m/platform/exynos/gpu_balance.c",
                                              "drivers/gpu/arm/t8xx/r12p0_m/platform/exynos/gpu_dvfs_api.c",
                                              "drivers/gpu/arm/t8xx/r12p0_m/platform/exynos/gpu_dvfs_handler.c",
                                              "drivers/gpu/arm/t8xx/r12p0_m/platform/exynos/gpu_dvfs_governor.c",
                                              "drivers/gpu/arm/t8xx/r12p0_m/platform/exynos/gpu_ipa.c",
                                              "drivers/gpu/arm/t8xx/r12p0_m/platform/exynos/gpu_hwcnt_sec.c",
                                              "drivers/gpu/arm/t8xx/r12p0_m/platform/exynos/gpu_pmqos.c",
                                              "drivers/gpu/arm/t8xx/r12p0_m/platform/exynos/gpu_exynos7890.c",
                                              "drivers/gpu/arm/t8xx/r12p0_m/platform/exynos/gpu_integration_callbacks.c",
                                              "drivers/gpu/drm/drm_edid.c",
                                              "drivers/gpu/drm/drm_gem_cma_helper.c",
                                              "drivers/gpu/drm/panel/panel-s6e8aa0.c",
                                              "drivers/gpu/drm/panel/panel-ld9040.c",
                                              "drivers/gpu/drm/panel/panel-simple.c",
                                              "drivers/gpu/drm/drm_mipi_dsi.c",
                                              "drivers/gpu/drm/tegra/gem.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_g2d.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_connector.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_gem.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_encoder.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_ipp.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_dmabuf.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_buf.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_fimc.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_core.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_fb.c",
                                              "drivers/gpu/drm/exynos/exynos_dp_core.c",
                                              "drivers/gpu/drm/exynos/exynos_dp_reg.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_dpi.c",
                                              "drivers/gpu/drm/exynos/exynos_mixer.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_drv.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_vidi.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_rotator.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_fimd.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_gsc.c",
                                              "drivers/gpu/drm/exynos/exynos_hdmi.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_fbdev.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_dsi.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_iommu.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_crtc.c",
                                              "drivers/gpu/drm/exynos/exynos_drm_plane.c",
                                              "drivers/gud/TlcTui/tlcTui.c",
                                              "drivers/gud/gud-exynos8890/sec-os-booster/sec_os_booster.c",
                                              "drivers/gud/gud-exynos8890/sec-os-ctrl/sec_os_ctrl.c",
                                              "drivers/gud/gud-exynos8890_kinibi311/sec-os-booster/sec_os_booster.c",
                                              "drivers/gud/gud-exynos8890_kinibi311/sec-os-ctrl/sec_os_ctrl.c",
                                              "drivers/hid/hid-core.c",
                                              "drivers/hid/hid-samsung.c",
                                              "drivers/hid/hid-ovr.c",
                                              "drivers/hid/hid-synaptics-bt.c",
                                              "drivers/hid/hid-madcatz.c",
                                              "drivers/hid/hid-input.c",
                                              "drivers/hwmon/sec_thermistor.c",
                                              "drivers/hwmon/pwm-fan.c",
                                              "drivers/hwmon/ntc_thermistor.c",
                                              "drivers/i2c/busses/i2c-exynos5.c",
                                              "drivers/i2c/busses/i2c-s3c2410.c",
                                              "drivers/ide/ide-dma.c",
                                              "drivers/ide/ide-cd.c",
                                              "drivers/ide/ide-cs.c",
                                              "drivers/ide/ide-iops.c",
                                              "drivers/ide/ide-pio-blacklist.c",
                                              "drivers/iio/adc/exynos_adc.c",
                                              "drivers/iio/light/cm36651.c",
                                              "drivers/iio/light/gp2ap020a00f.c",
                                              "drivers/input/evdev.c",
                                              "drivers/input/sec_cmd.c",
                                              "drivers/input/wacom/wacom_i2c_func.c",
                                              "drivers/input/wacom/wacom_i2c.c",
                                              "drivers/input/hall.c",
                                              "drivers/input/keyboard/max7359_keypad.c",
                                              "drivers/input/keyboard/atkbd.c",
                                              "drivers/input/keyboard/mcs_touchkey.c",
                                              "drivers/input/keyboard/abov_touchkey.c",
                                              "drivers/input/keyboard/samsung-keypad.c",
                                              "drivers/input/keyboard/cypress/cypress-touchkey.c",
                                              "drivers/input/misc/max8997_haptic.c",
                                              "drivers/input/misc/max77693-haptic.c",
                                              "drivers/input/touchscreen/mms114.c",
                                              "drivers/input/touchscreen/s3c2410_ts.c",
                                              "drivers/input/touchscreen/stm/fts8/fts_ts.c",
                                              "drivers/input/touchscreen/stm/fts8/fts_sec.c",
                                              "drivers/input/touchscreen/stm/fts7/fts_ts.c",
                                              "drivers/input/touchscreen/stm/fts7/fts_sec.c",
                                              "drivers/input/touchscreen/stm/fts_ts.c",
                                              "drivers/input/touchscreen/stm/fts_sec.c",
                                              "drivers/input/touchscreen/sec_ts/sec_ts_fn.c",
                                              "drivers/input/touchscreen/sec_ts/sec_ts.c",
                                              "drivers/input/touchscreen/mxt540e.c",
                                              "drivers/input/touchscreen/synaptics_dsx/synaptics_i2c_rmi.c",
                                              "drivers/input/touchscreen/synaptics_dsx/Multiverse/GMvBrane.c",
                                              "drivers/input/touchscreen/synaptics_dsx/Multiverse/GMvSystem.c",
                                              "drivers/input/touchscreen/sur40.c",
                                              "drivers/input/touchscreen/atmel_mxt_ts.c",
                                              "drivers/input/touchscreen/mxt_t/mxtt.c",
                                              "drivers/input/touchscreen/mxt_t/mxtt_sec.c",
                                              "drivers/input/touchscreen/synaptics_dsx2/synaptics_i2c_rmi.c",
                                              "drivers/input/touchscreen/mcs5000_ts.c",
                                              "drivers/input/mouse/elantech.c",
                                              "drivers/input/certify_hall.c",
                                              "drivers/iommu/iommu-traces.c",
                                              "drivers/iommu/exynos-iommu-log.c",
                                              "drivers/iommu/exynos-iommu.c",
                                              "drivers/iommu/exynos-iovmm.c",
                                              "drivers/irqchip/irq-s3c24xx.c",
                                              "drivers/irqchip/exynos-combiner.c",
                                              "drivers/leds/leds-bd2802.c",
                                              "drivers/leds/leds-max77854-rgb.c",
                                              "drivers/leds/leds-s2mpb02.c",
                                              "drivers/leds/leds-max8997.c",
                                              "drivers/mailbox/samsung/apm-exynos8890.c",
                                              "drivers/mailbox/samsung/apm-exynos.c",
                                              "drivers/mailbox/samsung/mailbox-exynos8.c",
                                              "drivers/md/dm-dirty.c",
                                              "drivers/media/platform/s5p-mfc/s5p_mfc_opr_v6.c",
                                              "drivers/media/platform/s5p-mfc/s5p_mfc_opr_v5.c",
                                              "drivers/media/platform/s5p-mfc/s5p_mfc_ctrl.c",
                                              "drivers/media/platform/s5p-mfc/s5p_mfc_cmd_v5.c",
                                              "drivers/media/platform/s5p-mfc/s5p_mfc_enc.c",
                                              "drivers/media/platform/s5p-mfc/s5p_mfc.c",
                                              "drivers/media/platform/s5p-mfc/s5p_mfc_cmd.c",
                                              "drivers/media/platform/s5p-mfc/s5p_mfc_dec.c",
                                              "drivers/media/platform/s5p-mfc/s5p_mfc_intr.c",
                                              "drivers/media/platform/s5p-mfc/s5p_mfc_pm.c",
                                              "drivers/media/platform/s5p-mfc/s5p_mfc_cmd_v6.c",
                                              "drivers/media/platform/s5p-mfc/s5p_mfc_opr.c",
                                              "drivers/media/platform/exynos-gsc/gsc-core.c",
                                              "drivers/media/platform/exynos-gsc/gsc-m2m.c",
                                              "drivers/media/platform/exynos-gsc/gsc-regs.c",
                                              "drivers/media/platform/s3c-camif/camif-core.c",
                                              "drivers/media/platform/s3c-camif/camif-regs.c",
                                              "drivers/media/platform/s3c-camif/camif-capture.c",
                                              "drivers/media/platform/s5p-g2d/g2d.c",
                                              "drivers/media/platform/s5p-g2d/g2d-hw.c",
                                              "drivers/media/platform/s5p-tv/hdmi_drv.c",
                                              "drivers/media/platform/s5p-tv/mixer_grp_layer.c",
                                              "drivers/media/platform/s5p-tv/sdo_drv.c",
                                              "drivers/media/platform/s5p-tv/hdmiphy_drv.c",
                                              "drivers/media/platform/s5p-tv/mixer_reg.c",
                                              "drivers/media/platform/s5p-tv/mixer_video.c",
                                              "drivers/media/platform/s5p-tv/mixer_vp_layer.c",
                                              "drivers/media/platform/s5p-tv/mixer_drv.c",
                                              "drivers/media/platform/s5p-tv/sii9234_drv.c",
                                              "drivers/media/platform/exynos4-is/fimc-is-errno.c",
                                              "drivers/media/platform/exynos4-is/fimc-is-param.c",
                                              "drivers/media/platform/exynos4-is/fimc-reg.c",
                                              "drivers/media/platform/exynos4-is/fimc-is-i2c.c",
                                              "drivers/media/platform/exynos4-is/mipi-csis.c",
                                              "drivers/media/platform/exynos4-is/fimc-is-sensor.c",
                                              "drivers/media/platform/exynos4-is/fimc-lite-reg.c",
                                              "drivers/media/platform/exynos4-is/fimc-isp.c",
                                              "drivers/media/platform/exynos4-is/fimc-lite.c",
                                              "drivers/media/platform/exynos4-is/fimc-is-regs.c",
                                              "drivers/media/platform/exynos4-is/fimc-m2m.c",
                                              "drivers/media/platform/exynos4-is/media-dev.c",
                                              "drivers/media/platform/exynos4-is/common.c",
                                              "drivers/media/platform/exynos4-is/fimc-capture.c",
                                              "drivers/media/platform/exynos4-is/fimc-core.c",
                                              "drivers/media/platform/exynos4-is/fimc-is.c",
                                              "drivers/media/platform/exynos4-is/fimc-isp-video.c",
                                              "drivers/media/platform/mem2mem_testdev.c",
                                              "drivers/media/platform/ti-vpe/vpe.c",
                                              "drivers/media/platform/s5p-jpeg/jpeg-hw-s5p.c",
                                              "drivers/media/platform/s5p-jpeg/jpeg-hw-exynos4.c",
                                              "drivers/media/platform/s5p-jpeg/jpeg-hw-exynos3250.c",
                                              "drivers/media/platform/s5p-jpeg/jpeg-core.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-video-ssvc0.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-video-sensor.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-video-mcsp.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-pipe.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-binary.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-i2c.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-mem.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-interface-wrap.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-video-scc.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-video-preprocessor.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/fimc-is-helper-i2c.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/fimc-is-device-sensor-peri.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/actuator/fimc-is-device-ak7348.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/actuator/fimc-is-device-dw9804.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/actuator/fimc-is-device-ak7345.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/actuator/fimc-is-device-dw9714.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/actuator/fimc-is-device-actuator-i2c.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/flash/fimc-is-device-rt5033.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/flash/fimc-is-device-lm3560.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/dphy/fimc-is-fpga-dphy.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/cis/fimc-is-cis-6b2.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/cis/fimc-is-cis-2p2.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/cis/fimc-is-cis-4h5.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/cis/fimc-is-cis.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/cis/fimc-is-cis-2p8.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/cis/fimc-is-cis-4h5yc.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/cis/fimc-is-cis-5e2.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/fimc-is-interface-actuator.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/modules/fimc-is-device-module-6b2.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/modules/fimc-is-device-module-base.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/modules/fimc-is-device-module-4h5.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/modules/fimc-is-device-module-4h5yc.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/modules/fimc-is-device-module-5e2.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/modules/fimc-is-device-module-2p8.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/modules/fimc-is-device-module-2p2.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/fimc-is-control-actuator.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/fimc-is-control-sensor.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module_framework/fimc-is-interface-sensor.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/csi/fimc-is-hw-csi-v3_4.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/csi/fimc-is-hw-csi-v3_2.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/csi2/fimc-is-hw-csi-v4_0.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/flite/fimc-is-hw-flite-v4_10_0.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/flite/fimc-is-hw-flite-v4_20_0.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/flite/fimc-is-hw-flite-v4_0.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/flite/fimc-is-hw-flite-v2_0.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-2t2.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-8b1.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-6d1.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-5e6.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-3h7_sunny.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-4e6.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-2p8.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-4e5.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-virtual-zebu.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-imx134.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-2p2_12m.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-4e6-c2.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-sr030-soc.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-imx175.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-4h5.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-2l1.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-3h5.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-sr352-soc.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-imx135.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-2p3.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-3h7.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-3m2.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-5e3.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-4h5yc.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-6b2.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-2p2.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-3l2.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-imx240.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-3p3.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-imx219.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-5e2.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-4ec-soc.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-6a3.c",
                                              "drivers/media/platform/exynos/fimc-is2/sensor/module/fimc-is-module-imx260.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-dt.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-device-ischain.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-video-ispc.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-dvfs.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-video-3aap.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-video-dis.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-subdev-ctrl.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-resourcemgr.c",
                                              "drivers/media/platform/exynos/fimc-is2/interface/fimc-is-interface-ddk.c",
                                              "drivers/media/platform/exynos/fimc-is2/interface/fimc-is-interface-fd.c",
                                              "drivers/media/platform/exynos/fimc-is2/interface/fimc-is-interface-library.c",
                                              "drivers/media/platform/exynos/fimc-is2/interface/fimc-is-interface-ischain.c",
                                              "drivers/media/platform/exynos/fimc-is2/interface/fimc-is-interface-vra.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-debug.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-video-vra.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-device-flite.c",
                                              "drivers/media/platform/exynos/fimc-is2/hardware/fimc-is-hw-3aa.c",
                                              "drivers/media/platform/exynos/fimc-is2/hardware/fimc-is-hw-vra.c",
                                              "drivers/media/platform/exynos/fimc-is2/hardware/fimc-is-hw-mcscaler.c",
                                              "drivers/media/platform/exynos/fimc-is2/hardware/fimc-is-hw-control.c",
                                              "drivers/media/platform/exynos/fimc-is2/hardware/fimc-is-hw-isp.c",
                                              "drivers/media/platform/exynos/fimc-is2/hardware/fimc-is-hw-tpu.c",
                                              "drivers/media/platform/exynos/fimc-is2/hardware/fimc-is-hw-dm.c",
                                              "drivers/media/platform/exynos/fimc-is2/hardware/fimc-is-hw-scp.c",
                                              "drivers/media/platform/exynos/fimc-is2/hardware/api/fimc-is-hw-api-vra.c",
                                              "drivers/media/platform/exynos/fimc-is2/hardware/api/fimc-is-hw-api-scp.c",
                                              "drivers/media/platform/exynos/fimc-is2/hardware/api/fimc-is-hw-api-common.c",
                                              "drivers/media/platform/exynos/fimc-is2/hardware/api/fimc-is-hw-api-mcscaler.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-clk-gate.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-video-ssvc2.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-video-ssvc3.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-video-ssvc1.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-video-mcs.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-video-scp.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-device-csi_v2.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-video-ispp.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-spi.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-video-isp.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-device-preprocessor.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-groupmgr.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-interface-wrap-fw.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-video-3aac.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-video.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-time.c",
                                              "drivers/media/platform/exynos/fimc-is2/vendor/mcd/fimc-is-device-ois_common.c",
                                              "drivers/media/platform/exynos/fimc-is2/vendor/mcd/fimc-is-device-eeprom.c",
                                              "drivers/media/platform/exynos/fimc-is2/vendor/mcd/fimc-is-sec-define.c",
                                              "drivers/media/platform/exynos/fimc-is2/vendor/mcd/fimc-is-device-ois_sa.c",
                                              "drivers/media/platform/exynos/fimc-is2/vendor/mcd/fimc-is-device-from.c",
                                              "drivers/media/platform/exynos/fimc-is2/vendor/mcd/fimc-is-companion_c2.c",
                                              "drivers/media/platform/exynos/fimc-is2/vendor/mcd/fimc-is-ncp6335b.c",
                                              "drivers/media/platform/exynos/fimc-is2/vendor/mcd/fimc-is-companion_c3.c",
                                              "drivers/media/platform/exynos/fimc-is2/vendor/mcd/fimc-is-vender.c",
                                              "drivers/media/platform/exynos/fimc-is2/vendor/mcd/fimc-is-sysfs.c",
                                              "drivers/media/platform/exynos/fimc-is2/vendor/mcd/crc32.c",
                                              "drivers/media/platform/exynos/fimc-is2/vendor/mcd/fimc-is-device-ois_s4.c",
                                              "drivers/media/platform/exynos/fimc-is2/vendor/mcd/fimc-is-fan53555.c",
                                              "drivers/media/platform/exynos/fimc-is2/vendor/mcd/fimc-is-companion.c",
                                              "drivers/media/platform/exynos/fimc-is2/vendor/mcd/fimc-is-device-af.c",
                                              "drivers/media/platform/exynos/fimc-is2/vendor/default/fimc-is-vender.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-device-csi.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/setup-fimc-is-preprocessor.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v4_0_0/fimc-is-hw-chain.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v4_0_0/fimc-is-hw-pwr.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v4_0_0/fimc-is-hw-dvfs.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/setup-fimc-is.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v4_3_0/fimc-is-subdev-3aa.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v4_3_0/fimc-is-subdev-ixc.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v4_3_0/fimc-is-subdev-dis.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v4_3_0/fimc-is-subdev-isp.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v4_3_0/fimc-is-subdev-3ac.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v4_3_0/fimc-is-hw-chain.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v4_3_0/fimc-is-subdev-mcs.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v4_3_0/fimc-is-subdev-mcsp.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v4_3_0/fimc-is-subdev-ixp.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v4_3_0/fimc-is-hw-pwr.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v4_3_0/fimc-is-hw-dvfs.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v4_3_0/fimc-is-subdev-3ap.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v4_3_0/fimc-is-subdev-vra.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/setup-fimc-is-sensor.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/setup-fimc-is-module.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v3_1_1/fimc-is-hw-chain.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v3_1_1/fimc-is-hw-pwr.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v3_1_1/fimc-is-hw-dvfs.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v3_11_0/fimc-is-subdev-3aa.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v3_11_0/fimc-is-subdev-ixc.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v3_11_0/fimc-is-subdev-isp.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v3_11_0/fimc-is-subdev-3ac.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v3_11_0/fimc-is-hw-chain.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v3_11_0/fimc-is-subdev-ixp.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v3_11_0/fimc-is-hw-pwr.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v3_11_0/fimc-is-hw-dvfs.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v3_11_0/fimc-is-subdev-3ap.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v3_11_0/fimc-is-subdev-vra.c",
                                              "drivers/media/platform/exynos/fimc-is2/ischain/fimc-is-v3_11_0/fimc-is-subdev-scp.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-video-3aa.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-framemgr.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-device-sensor.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-core.c",
                                              "drivers/media/platform/exynos/fimc-is2/fimc-is-interface.c",
                                              "drivers/media/platform/exynos/mfc/s5p_mfc_nal_q.c",
                                              "drivers/media/platform/exynos/mfc/s5p_mfc_opr_v6.c",
                                              "drivers/media/platform/exynos/mfc/s5p_mfc_buf.c",
                                              "drivers/media/platform/exynos/mfc/s5p_mfc_ctrl.c",
                                              "drivers/media/platform/exynos/mfc/s5p_mfc_enc.c",
                                              "drivers/media/platform/exynos/mfc/s5p_mfc.c",
                                              "drivers/media/platform/exynos/mfc/s5p_mfc_utils.c",
                                              "drivers/media/platform/exynos/mfc/s5p_mfc_dec.c",
                                              "drivers/media/platform/exynos/mfc/s5p_mfc_enc_param.c",
                                              "drivers/media/platform/exynos/mfc/s5p_mfc_intr.c",
                                              "drivers/media/platform/exynos/mfc/s5p_mfc_pm.c",
                                              "drivers/media/platform/exynos/mfc/s5p_mfc_cmd_v6.c",
                                              "drivers/media/platform/exynos/mfc/s5p_mfc_mem.c",
                                              "drivers/media/platform/exynos/mfc/s5p_mfc_qos.c",
                                              "drivers/media/platform/exynos/mfc/s5p_mfc_inst.c",
                                              "drivers/media/platform/exynos/scaler/scaler-core.c",
                                              "drivers/media/platform/exynos/scaler/scaler-regs.c",
                                              "drivers/media/platform/exynos/fimg2d_v5/fimg2d_clk.c",
                                              "drivers/media/platform/exynos/fimg2d_v5/fimg2d5x_hw.c",
                                              "drivers/media/platform/exynos/fimg2d_v5/g2d1shot_helper.c",
                                              "drivers/media/platform/exynos/fimg2d_v5/fimg2d_ctx.c",
                                              "drivers/media/platform/exynos/fimg2d_v5/fimg2d5x_blt.c",
                                              "drivers/media/platform/exynos/fimg2d_v5/g2d1shot_hw5x.c",
                                              "drivers/media/platform/exynos/fimg2d_v5/fimg2d_drv.c",
                                              "drivers/media/platform/exynos/fimg2d_v5/fimg2d_helper.c",
                                              "drivers/media/platform/exynos/fimg2d_v5/fimg2d_cache.c",
                                              "drivers/media/platform/exynos/fimg2d_v5/g2d1shot_drv.c",
                                              "drivers/media/platform/exynos/mdev/exynos-mdev.c",
                                              "drivers/media/platform/exynos/hevc/hevc_mem.c",
                                              "drivers/media/platform/exynos/hevc/hevc_opr.c",
                                              "drivers/media/platform/exynos/hevc/hevc_qos.c",
                                              "drivers/media/platform/exynos/hevc/hevc_cmd.c",
                                              "drivers/media/platform/exynos/hevc/hevc_ctrl.c",
                                              "drivers/media/platform/exynos/hevc/hevc_reg.c",
                                              "drivers/media/platform/exynos/hevc/hevc_pm.c",
                                              "drivers/media/platform/exynos/hevc/hevc_intr.c",
                                              "drivers/media/platform/exynos/hevc/hevc_inst.c",
                                              "drivers/media/platform/exynos/hevc/hevc.c",
                                              "drivers/media/platform/exynos/hevc/hevc_dec.c",
                                              "drivers/media/platform/exynos/smfc/smfc.c",
                                              "drivers/media/platform/exynos/smfc/smfc-regs.c",
                                              "drivers/media/platform/exynos/smfc/smfc-stream-parser.c",
                                              "drivers/media/platform/exynos/smfc/smfc-v4l2-ioctls.c",
                                              "drivers/media/m2m1shot.c",
                                              "drivers/media/tuners/tuner-types.c",
                                              "drivers/media/radio/si470x/radio-si470x-i2c.c",
                                              "drivers/media/dvb-frontends/s5h1411.c",
                                              "drivers/media/dvb-frontends/dvb-pll.c",
                                              "drivers/media/dvb-frontends/stv0299.c",
                                              "drivers/media/dvb-frontends/s5h1432.c",
                                              "drivers/media/dvb-frontends/s5h1420.c",
                                              "drivers/media/dvb-frontends/s5h1409.c",
                                              "drivers/media/dvb-frontends/mt352.c",
                                              "drivers/media/rc/imon.c",
                                              "drivers/media/pci/saa7146/hexium_gemini.c",
                                              "drivers/media/pci/cx88/cx88-cards.c",
                                              "drivers/media/pci/cx88/cx88-dvb.c",
                                              "drivers/media/pci/cx88/cx88-mpeg.c",
                                              "drivers/media/pci/cx18/cx18-driver.c",
                                              "drivers/media/pci/cx18/cx18-dvb.c",
                                              "drivers/media/pci/cx18/cx18-cards.c",
                                              "drivers/media/pci/ttpci/budget.c",
                                              "drivers/media/pci/saa7134/saa7134-i2c.c",
                                              "drivers/media/pci/bt8xx/dst.c",
                                              "drivers/media/pci/bt8xx/dvb-bt8xx.c",
                                              "drivers/media/pci/bt8xx/bttv-cards.c",
                                              "drivers/media/common/b2c2/flexcop-fe-tuner.c",
                                              "drivers/media/common/tveeprom.c",
                                              "drivers/media/v4l2-core/videobuf2-memops.c",
                                              "drivers/media/v4l2-core/v4l2-mem2mem.c",
                                              "drivers/media/v4l2-core/videobuf2-vmalloc.c",
                                              "drivers/media/v4l2-core/videobuf2-dma-sg.c",
                                              "drivers/media/v4l2-core/v4l2-of.c",
                                              "drivers/media/v4l2-core/videobuf2-ion.c",
                                              "drivers/media/v4l2-core/videobuf2-dma-contig.c",
                                              "drivers/media/v4l2-core/videobuf2-core.c",
                                              "drivers/media/i2c/noon010pc30.c",
                                              "drivers/media/i2c/sr030pc30.c",
                                              "drivers/media/i2c/s5k5baf.c",
                                              "drivers/media/i2c/s5k6a3.c",
                                              "drivers/media/i2c/s5k4ecgx.c",
                                              "drivers/media/i2c/s5k6aa.c",
                                              "drivers/media/i2c/s5c73m3/s5c73m3-core.c",
                                              "drivers/media/i2c/s5c73m3/s5c73m3-ctrls.c",
                                              "drivers/media/i2c/s5c73m3/s5c73m3-spi.c",
                                              "drivers/media/i2c/m5mols/m5mols_capture.c",
                                              "drivers/media/i2c/m5mols/m5mols_core.c",
                                              "drivers/media/i2c/m5mols/m5mols_controls.c",
                                              "drivers/media/m2m1shot2.c",
                                              "drivers/media/m2m1shot-helper.c",
                                              "drivers/media/m2m1shot2-testdev.c",
                                              "drivers/media/tdmb/tdmb_tsi_qc.c",
                                              "drivers/media/tdmb/tdmb_i2c.c",
                                              "drivers/media/tdmb/tdmb_spi.c",
                                              "drivers/media/tdmb/tdmb_ebi.c",
                                              "drivers/media/tdmb/tdmb_data.c",
                                              "drivers/media/tdmb/tdmb_port_mtv319.c",
                                              "drivers/media/tdmb/tdmb_tsi_slsi.c",
                                              "drivers/media/tdmb/tdmb.c",
                                              "drivers/media/tdmb/tdmb_port_fc8080.c",
                                              "drivers/media/m2m1shot-testdev.c",
                                              "drivers/media/usb/uvc/uvc_driver.c",
                                              "drivers/media/usb/dvb-usb/vp7045-fe.c",
                                              "drivers/media/usb/pwc/pwc-if.c",
                                              "drivers/media/usb/gspca/vc032x.c",
                                              "drivers/media/usb/au0828/au0828-input.c",
                                              "drivers/media/usb/em28xx/em28xx-camera.c",
                                              "drivers/media/usb/dvb-usb-v2/anysee.c",
                                              "drivers/mfd/max8998-irq.c",
                                              "drivers/mfd/sec-irq.c",
                                              "drivers/mfd/max8998.c",
                                              "drivers/mfd/max14577.c",
                                              "drivers/mfd/max77854.c",
                                              "drivers/mfd/s2mpb02-core.c",
                                              "drivers/mfd/max77833-irq.c",
                                              "drivers/mfd/s2mu003_irq.c",
                                              "drivers/mfd/max77833.c",
                                              "drivers/mfd/s2mpb02-irq.c",
                                              "drivers/mfd/s2mu003_core.c",
                                              "drivers/mfd/max77686.c",
                                              "drivers/mfd/max77854-irq.c",
                                              "drivers/mfd/max8997.c",
                                              "drivers/mfd/sec-core.c",
                                              "drivers/mfd/max77693.c",
                                              "drivers/mfd/max8997-irq.c",
                                              "drivers/misc/modem_v1/link_device_memory_flow_control.c",
                                              "drivers/misc/modem_v1/link_ctrlmsg_iosm.c",
                                              "drivers/misc/modem_v1/modem_main.c",
                                              "drivers/misc/modem_v1/modem_ctrl_ss310ap.c",
                                              "drivers/misc/modem_v1/link_device_memory_main.c",
                                              "drivers/misc/modem_v1/link_device_memory_command.c",
                                              "drivers/misc/modem_v1/modem_utils.c",
                                              "drivers/misc/modem_v1/modem_io_device.c",
                                              "drivers/misc/modem_v1/link_device_memory_debug.c",
                                              "drivers/misc/modem_v1/modem_variation.c",
                                              "drivers/misc/modem_v1/modem_pktlog.c",
                                              "drivers/misc/modem_v1/link_device_memory_snapshot.c",
                                              "drivers/misc/modem_v1/link_device_memory_sbd.c",
                                              "drivers/misc/modem_v1/link_device_shmem.c",
                                              "drivers/misc/tzic64.c",
                                              "drivers/misc/fsa9480.c",
                                              "drivers/misc/mcu_ipc/mcu_ipc.c",
                                              "drivers/misc/mcu_ipc/shm_ipc.c",
                                              "drivers/mmc/card/block.c",
                                              "drivers/mmc/host/s3cmci.c",
                                              "drivers/mmc/host/sdhci-pci.c",
                                              "drivers/mmc/host/dw_mmc-exynos.c",
                                              "drivers/mmc/host/sdhci-s3c.c",
                                              "drivers/mmc/host/dw_mmc.c",
                                              "drivers/motor/max77854_haptic.c",
                                              "drivers/mtd/onenand/onenand_base.c",
                                              "drivers/mtd/onenand/samsung.c",
                                              "drivers/mtd/onenand/generic.c",
                                              "drivers/mtd/onenand/onenand_bbt.c",
                                              "drivers/mtd/nand/nand_base.c",
                                              "drivers/mtd/nand/denali.c",
                                              "drivers/mtd/nand/nand_ids.c",
                                              "drivers/mtd/nand/s3c2410.c",
                                              "drivers/mtd/chips/cfi_cmdset_0002.c",
                                              "drivers/mtd/ubi/io.c",
                                              "drivers/muic/muic-core.c",
                                              "drivers/muic/max77833-muic.c",
                                              "drivers/muic/max77843-muic.c",
                                              "drivers/muic/max77833-muic-ccic.c",
                                              "drivers/muic/universal/muic_regmap_max77854.c",
                                              "drivers/muic/universal/muic_apis.c",
                                              "drivers/muic/universal/muic_regmap_max77849.c",
                                              "drivers/muic/universal/muic_regmap.c",
                                              "drivers/muic/universal/muic_hv.c",
                                              "drivers/muic/universal/muic_i2c.c",
                                              "drivers/muic/universal/muic_dt.c",
                                              "drivers/muic/universal/muic_state.c",
                                              "drivers/muic/universal/muic_sysfs.c",
                                              "drivers/muic/universal/muic_regmap_s2mm001b.c",
                                              "drivers/muic/universal/muic_debug.c",
                                              "drivers/muic/universal/muic_task.c",
                                              "drivers/muic/universal/muic_regmap_sm5703.c",
                                              "drivers/muic/universal/muic_vps.c",
                                              "drivers/muic/universal/muic_ccic.c",
                                              "drivers/muic/universal/muic_usb.c",
                                              "drivers/muic/max77804-muic.c",
                                              "drivers/muic/max77843-muic-afc.c",
                                              "drivers/muic/tsu6721-muic.c",
                                              "drivers/muic/max77833-muic-afc.c",
                                              "drivers/muic/fsa9480.c",
                                              "drivers/muic/max77888-muic.c",
                                              "drivers/muic/max77854-muic-afc.c",
                                              "drivers/muic/max77854-muic.c",
                                              "drivers/muic/max77828-muic.c",
                                              "drivers/muic/max77804k-muic.c",
                                              "drivers/muic/s2mm001.c",
                                              "drivers/net/wireless/hostap/hostap_plx.c",
                                              "drivers/net/wireless/hostap/hostap_pci.c",
                                              "drivers/net/wireless/orinoco/orinoco_usb.c",
                                              "drivers/net/wireless/orinoco/hw.c",
                                              "drivers/net/wireless/orinoco/orinoco_cs.c",
                                              "drivers/net/wireless/orinoco/orinoco_pci.c",
                                              "drivers/net/wireless/at76c50x-usb.c",
                                              "drivers/net/wireless/rt2x00/rt2800usb.c",
                                              "drivers/net/wireless/rt2x00/rt73usb.c",
                                              "drivers/net/wireless/ath/ath9k/pci.c",
                                              "drivers/net/wireless/rtlwifi/rtl8723ae/hw.c",
                                              "drivers/net/wireless/rtlwifi/rtl8723be/hw.c",
                                              "drivers/net/wireless/bcmdhd4358/dhd_custom_exynos.c",
                                              "drivers/net/wireless/bcmdhd4358/dhd_custom_msm.c",
                                              "drivers/net/wireless/bcmdhd4358/wl_android.c",
                                              "drivers/net/wireless/bcmdhd4358/dhd_custom_sec.c",
                                              "drivers/net/wireless/bcmdhd4358/dhd_pcie_linux.c",
                                              "drivers/net/wireless/bcmdhd4359/dhd_custom_cis.c",
                                              "drivers/net/wireless/bcmdhd4359/dhd_custom_exynos.c",
                                              "drivers/net/wireless/bcmdhd4359/dhd_custom_msm.c",
                                              "drivers/net/wireless/bcmdhd4359/wl_android.c",
                                              "drivers/net/wireless/bcmdhd4359/dhd_pcie_linux.c",
                                              "drivers/net/ethernet/samsung/sxgbe/sxgbe_xpcs.c",
                                              "drivers/net/ethernet/samsung/sxgbe/sxgbe_desc.c",
                                              "drivers/net/ethernet/samsung/sxgbe/sxgbe_dma.c",
                                              "drivers/net/ethernet/samsung/sxgbe/sxgbe_platform.c",
                                              "drivers/net/ethernet/samsung/sxgbe/sxgbe_mdio.c",
                                              "drivers/net/ethernet/samsung/sxgbe/sxgbe_core.c",
                                              "drivers/net/ethernet/samsung/sxgbe/sxgbe_mtl.c",
                                              "drivers/net/ethernet/samsung/sxgbe/sxgbe_ethtool.c",
                                              "drivers/net/ethernet/samsung/sxgbe/sxgbe_main.c",
                                              "drivers/net/tun.c",
                                              "drivers/net/usb/cdc_ether.c",
                                              "drivers/net/usb/ax88179_178a.c",
                                              "drivers/net/usb/r8152.c",
                                              "drivers/net/usb/qmi_wwan.c",
                                              "drivers/net/usb/kalmia.c",
                                              "drivers/nfc/sec_coffee.c",
                                              "drivers/nfc/sec_nfc.c",
                                              "drivers/nfc/ese_p3.c",
                                              "drivers/nfc/secnfc/sec_nfc.c",
                                              "drivers/nfc/secnfc/sec_nfc_fn.c",
                                              "drivers/of/of_reserved_mem.c",
                                              "drivers/of/platform.c",
                                              "drivers/pci/quirks.c",
                                              "drivers/pci/host/pcie-designware.c",
                                              "drivers/pci/host/pci-exynos8890.c",
                                              "drivers/pci/host/pci-exynos.c",
                                              "drivers/pci/host/pci-exynos8890_cal.c",
                                              "drivers/phy/phy-samsung-usb2.c",
                                              "drivers/phy/phy-samsung-usb3-cal.c",
                                              "drivers/phy/phy-exynos5250-usb2.c",
                                              "drivers/phy/phy-exynos4x12-usb2.c",
                                              "drivers/phy/phy-sun4i-usb.c",
                                              "drivers/phy/phy-exynos-dp-video.c",
                                              "drivers/phy/phy-exynos5-usbdrd.c",
                                              "drivers/phy/phy-exynos5250-sata.c",
                                              "drivers/phy/phy-exynos4210-usb2.c",
                                              "drivers/phy/phy-exynos-mipi-video.c",
                                              "drivers/phy/phy-exynos-mipi.c",
                                              "drivers/phy/phy-exynos-usbdrd.c",
                                              "drivers/phy/phy-s5pv210-usb2.c",
                                              "drivers/pinctrl/samsung/pinctrl-exynos.c",
                                              "drivers/pinctrl/samsung/pinctrl-s3c24xx.c",
                                              "drivers/pinctrl/samsung/secgpio_dvs.c",
                                              "drivers/pinctrl/samsung/pinctrl-samsung.c",
                                              "drivers/pinctrl/samsung/pinctrl-s3c64xx.c",
                                              "drivers/pinctrl/samsung/pinctrl-exynos5440.c",
                                              "drivers/pinctrl/pinctrl-rockchip.c",
                                              "drivers/platform/x86/samsung-laptop.c",
                                              "drivers/platform/x86/samsung-q10.c",
                                              "drivers/platform/chrome/chromeos_pstore.c",
                                              "drivers/platform/chrome/chromeos_laptop.c",
                                              "drivers/power/max8903_charger.c",
                                              "drivers/power/max17058_fuelgauge.c",
                                              "drivers/power/charger-manager.c",
                                              "drivers/power/sec_fuelgauge.c",
                                              "drivers/power/max17042_battery.c",
                                              "drivers/power/max8997_charger.c",
                                              "drivers/power/max14577_charger.c",
                                              "drivers/power/max8998_charger.c",
                                              "drivers/power/max17040_battery.c",
                                              "drivers/power/ds2760_battery.c",
                                              "drivers/pwm/pwm-samsung.c",
                                              "drivers/regulator/s2mpb02-regulator.c",
                                              "drivers/regulator/max8998.c",
                                              "drivers/regulator/s2mps15.c",
                                              "drivers/regulator/s2mpb01.c",
                                              "drivers/regulator/max14577.c",
                                              "drivers/regulator/s2mpu03.c",
                                              "drivers/regulator/max77854.c",
                                              "drivers/regulator/s2mps16.c",
                                              "drivers/regulator/lp3971.c",
                                              "drivers/regulator/s2mps13.c",
                                              "drivers/regulator/max77833.c",
                                              "drivers/regulator/s2mps11.c",
                                              "drivers/regulator/s2mu003-regulator.c",
                                              "drivers/regulator/s5m8767.c",
                                              "drivers/regulator/max77802.c",
                                              "drivers/regulator/max77686.c",
                                              "drivers/regulator/s2mpa01.c",
                                              "drivers/regulator/max8997.c",
                                              "drivers/regulator/max77693.c",
                                              "drivers/regulator/max8952.c",
                                              "drivers/rtc/exynos_persistent_clock.c",
                                              "drivers/rtc/rtc-max8998.c",
                                              "drivers/rtc/rtc-s5m.c",
                                              "drivers/rtc/rtc-max8997.c",
                                              "drivers/rtc/rtc-s3c.c",
                                              "drivers/rtc/rtc-max77802.c",
                                              "drivers/rtc/rtc-sec.c",
                                              "drivers/rtc/rtc-max77686.c",
                                              "drivers/scsi/ufs/ufshcd-pci.c",
                                              "drivers/scsi/ufs/ufs-exynos.c",
                                              "drivers/scsi/ufs/ufshcd-pltfrm.c",
                                              "drivers/scsi/ufs/ufs_quirks.c",
                                              "drivers/scsi/ufs/ufshcd.c",
                                              "drivers/sensorhub/atmel/max86900.c",
                                              "drivers/sensorhub/atmel/ssp_spi.c",
                                              "drivers/sensorhub/atmel/ssp_sysfs.c",
                                              "drivers/sensorhub/atmel/ssp_input.c",
                                              "drivers/sensorhub/atmel/ssp_sensorhub.c",
                                              "drivers/sensorhub/atmel/factory/mcu_atuc128l5har.c",
                                              "drivers/sensorhub/atmel/factory/magnetic_ak8963c.c",
                                              "drivers/sensorhub/atmel/factory/gyro_bmi058.c",
                                              "drivers/sensorhub/atmel/factory/prox_max88005.c",
                                              "drivers/sensorhub/atmel/factory/prox_max88920.c",
                                              "drivers/sensorhub/atmel/factory/accel_mpu6050.c",
                                              "drivers/sensorhub/atmel/factory/gyro_lsm330.c",
                                              "drivers/sensorhub/atmel/factory/gesture_max88922.c",
                                              "drivers/sensorhub/atmel/factory/gyro_mpu6500.c",
                                              "drivers/sensorhub/atmel/factory/prox_cm36651.c",
                                              "drivers/sensorhub/atmel/factory/accel_bmi058.c",
                                              "drivers/sensorhub/atmel/factory/gyro_mpu6050.c",
                                              "drivers/sensorhub/atmel/factory/mcu_at32uc3l0128.c",
                                              "drivers/sensorhub/atmel/factory/light_max88921.c",
                                              "drivers/sensorhub/atmel/factory/gesture_max88920.c",
                                              "drivers/sensorhub/atmel/factory/pressure_bmp182.c",
                                              "drivers/sensorhub/atmel/factory/light_cm36651.c",
                                              "drivers/sensorhub/atmel/factory/pressure_lps25h.c",
                                              "drivers/sensorhub/atmel/factory/temphumidity_shtc1.c",
                                              "drivers/sensorhub/atmel/factory/accel_mpu6500.c",
                                              "drivers/sensorhub/atmel/factory/accel_lsm330.c",
                                              "drivers/sensorhub/atmel/factory/magnetic_yas532.c",
                                              "drivers/sensorhub/atmel/factory/light_cm3320.c",
                                              "drivers/sensorhub/atmel/factory/gesture_tmg399x.c",
                                              "drivers/sensorhub/atmel/factory/light_tmg399x.c",
                                              "drivers/sensorhub/atmel/ssp_coms.c",
                                              "drivers/sensorhub/atmel/ssp_iio_trigger.c",
                                              "drivers/sensorhub/atmel/ssp_i2c.c",
                                              "drivers/sensorhub/atmel/ssp_firmware.c",
                                              "drivers/sensorhub/atmel/ssp_debug.c",
                                              "drivers/sensorhub/atmel/ssp_ak8963c.c",
                                              "drivers/sensorhub/atmel/ssp_iio_ring.c",
                                              "drivers/sensorhub/atmel/sensors_core.c",
                                              "drivers/sensorhub/atmel/ssp_data.c",
                                              "drivers/sensorhub/atmel/ssp_c12sd.c",
                                              "drivers/sensorhub/atmel/ssp_dev.c",
                                              "drivers/sensorhub/stm/max86900.c",
                                              "drivers/sensorhub/stm/ssp_misc.c",
                                              "drivers/sensorhub/stm/ssp_spi.c",
                                              "drivers/sensorhub/stm/ssp_sysfs.c",
                                              "drivers/sensorhub/stm/max86902.c",
                                              "drivers/sensorhub/stm/ssp_sensorhub.c",
                                              "drivers/sensorhub/stm/factory/mcu_atuc128l5har.c",
                                              "drivers/sensorhub/stm/factory/magnetic_ak09911.c",
                                              "drivers/sensorhub/stm/factory/prox_tmg399x.c",
                                              "drivers/sensorhub/stm/factory/accel_general.c",
                                              "drivers/sensorhub/stm/factory/gyro_mpu6500.c",
                                              "drivers/sensorhub/stm/factory/gyro_bmi168.c",
                                              "drivers/sensorhub/stm/factory/gyro_general.c",
                                              "drivers/sensorhub/stm/factory/accel_mpu6500.c",
                                              "drivers/sensorhub/stm/factory/gesture_tmg399x.c",
                                              "drivers/sensorhub/stm/factory/light_tmg399x.c",
                                              "drivers/sensorhub/stm/factory/pressure_bmp280.c",
                                              "drivers/sensorhub/stm/factory/accel_bmi168.c",
                                              "drivers/sensorhub/stm/ssp_iio.c",
                                              "drivers/sensorhub/stm/sx9310.c",
                                              "drivers/sensorhub/stm/ssp_firmware.c",
                                              "drivers/sensorhub/stm/ssp_debug.c",
                                              "drivers/sensorhub/stm/sensors_core.c",
                                              "drivers/sensorhub/stm/ssp_data.c",
                                              "drivers/sensorhub/stm/ssp_dev.c",
                                              "drivers/sensorhub/brcm/max86900.c",
                                              "drivers/sensorhub/brcm/ssp_misc.c",
                                              "drivers/sensorhub/brcm/ssp_spi.c",
                                              "drivers/sensorhub/brcm/sx9320.c",
                                              "drivers/sensorhub/brcm/ssp_sysfs.c",
                                              "drivers/sensorhub/brcm/ssp_input.c",
                                              "drivers/sensorhub/brcm/max86902.c",
                                              "drivers/sensorhub/brcm/ssp_bbd.c",
                                              "drivers/sensorhub/brcm/ssp_dump.c",
                                              "drivers/sensorhub/brcm/ssp_ak8963c_doeplus.c",
                                              "drivers/sensorhub/brcm/ssp_sensorhub.c",
                                              "drivers/sensorhub/brcm/factory/mcu_atuc128l5har.c",
                                              "drivers/sensorhub/brcm/factory/gyro_k330.c",
                                              "drivers/sensorhub/brcm/factory/magnetic_ak8963c.c",
                                              "drivers/sensorhub/brcm/factory/gyro_bmi058.c",
                                              "drivers/sensorhub/brcm/factory/magnetic_common.c",
                                              "drivers/sensorhub/brcm/factory/magnetic_ak09911.c",
                                              "drivers/sensorhub/brcm/factory/prox_tmg399x.c",
                                              "drivers/sensorhub/brcm/factory/mcu_bcm4774.c",
                                              "drivers/sensorhub/brcm/factory/prox_max88920.c",
                                              "drivers/sensorhub/brcm/factory/magnetic_yas537.c",
                                              "drivers/sensorhub/brcm/factory/accel_mpu6050.c",
                                              "drivers/sensorhub/brcm/factory/gesture_max88922.c",
                                              "drivers/sensorhub/brcm/factory/light_colorid.c",
                                              "drivers/sensorhub/brcm/factory/mcu_stm32f401.c",
                                              "drivers/sensorhub/brcm/factory/gyro_mpu6500.c",
                                              "drivers/sensorhub/brcm/factory/accel_bmi058.c",
                                              "drivers/sensorhub/brcm/factory/gyro_mpu6050.c",
                                              "drivers/sensorhub/brcm/factory/mcu_at32uc3l0128.c",
                                              "drivers/sensorhub/brcm/factory/magnetic_yas539.c",
                                              "drivers/sensorhub/brcm/factory/light_max88921.c",
                                              "drivers/sensorhub/brcm/factory/gesture_max88920.c",
                                              "drivers/sensorhub/brcm/factory/pressure_bmp182.c",
                                              "drivers/sensorhub/brcm/factory/accel_icm20610.c",
                                              "drivers/sensorhub/brcm/factory/pressure_lps25h.c",
                                              "drivers/sensorhub/brcm/factory/temphumidity_shtc1.c",
                                              "drivers/sensorhub/brcm/factory/grip_sx9306.c",
                                              "drivers/sensorhub/brcm/factory/gyro_icm20610.c",
                                              "drivers/sensorhub/brcm/factory/light_cm3323.c",
                                              "drivers/sensorhub/brcm/factory/accel_mpu6500.c",
                                              "drivers/sensorhub/brcm/factory/accel_k6ds3tr.c",
                                              "drivers/sensorhub/brcm/factory/magnetic_yas532.c",
                                              "drivers/sensorhub/brcm/factory/light_cm3320.c",
                                              "drivers/sensorhub/brcm/factory/gesture_tmg399x.c",
                                              "drivers/sensorhub/brcm/factory/light_tmg399x.c",
                                              "drivers/sensorhub/brcm/factory/accel_k330.c",
                                              "drivers/sensorhub/brcm/factory/gyro_k6ds3tr.c",
                                              "drivers/sensorhub/brcm/ssp_iio_trigger.c",
                                              "drivers/sensorhub/brcm/sx9310.c",
                                              "drivers/sensorhub/brcm/ssp_i2c.c",
                                              "drivers/sensorhub/brcm/ssp_firmware.c",
                                              "drivers/sensorhub/brcm/ssp_debug.c",
                                              "drivers/sensorhub/brcm/ssp_ak8963c.c",
                                              "drivers/sensorhub/brcm/ssp_iio_ring.c",
                                              "drivers/sensorhub/brcm/sensors_core.c",
                                              "drivers/sensorhub/brcm/ssp_data.c",
                                              "drivers/sensorhub/brcm/ssp_c12sd.c",
                                              "drivers/sensorhub/brcm/hrmsensor.c",
                                              "drivers/sensorhub/brcm/ssp_dev.c",
                                              "drivers/soc/samsung/exynos-hotplug_governor.c",
                                              "drivers/soc/samsung/exynos-cpu_hotplug.c",
                                              "drivers/soc/samsung/exynos-chipid.c",
                                              "drivers/soc/samsung/exynos-smc.c",
                                              "drivers/soc/samsung/exynos-powermode.c",
                                              "drivers/soc/samsung/exynos-reboot.c",
                                              "drivers/soc/samsung/ect_parser.c",
                                              "drivers/soc/samsung/exynos-el3_mon.c",
                                              "drivers/soc/samsung/exynos-pmu.c",
                                              "drivers/soc/samsung/exynos-mcinfo.c",
                                              "drivers/soc/samsung/pm_domains/pm_domains_sysfs.c",
                                              "drivers/soc/samsung/pm_domains/pm_domains-cal.c",
                                              "drivers/soc/samsung/exynos-pm-dvs.c",
                                              "drivers/soc/samsung/exynos-pm.c",
                                              "drivers/soc/samsung/exynos_otp.c",
                                              "drivers/soc/samsung/pwrcal/S5E8890/S5E8890-cmu.c",
                                              "drivers/soc/samsung/pwrcal/S5E8890/S5E8890-dfs.c",
                                              "drivers/soc/samsung/pwrcal/S5E8890/S5E8890-drampara.c",
                                              "drivers/soc/samsung/pwrcal/S5E8890/S5E8890-asv.c",
                                              "drivers/soc/samsung/secmem.c",
                                              "drivers/spi/spi-s3c64xx.c",
                                              "drivers/staging/media/lirc/lirc_imon.c",
                                              "drivers/staging/android/ion/exynos/exynos_ion.c",
                                              "drivers/staging/panel/panel.c",
                                              "drivers/staging/samsung/sec_data_param.c",
                                              "drivers/staging/samsung/sec_memory.c",
                                              "drivers/staging/samsung/sec_upload.c",
                                              "drivers/staging/samsung/sec_debug.c",
                                              "drivers/staging/samsung/sec_misc.c",
                                              "drivers/staging/samsung/sec_reboot.c",
                                              "drivers/staging/samsung/sec_bsp.c",
                                              "drivers/staging/samsung/sec_hotplug.c",
                                              "drivers/staging/samsung/sec_getlog.c",
                                              "drivers/staging/samsung/sec_sysfs.c",
                                              "drivers/staging/samsung/sec_batt.c",
                                              "drivers/staging/samsung/sentinel.c",
                                              "drivers/staging/samsung/sec_argos.c",
                                              "drivers/staging/samsung/sec_initcall_debug.c",
                                              "drivers/staging/samsung/sec_nad.c",
                                              "drivers/staging/vnswap/vnswap.c",
                                              "drivers/switch/switch-arizona.c",
                                              "drivers/switch/switch-arizona_ptt.c",
                                              "drivers/thermal/ipa.c",
                                              "drivers/thermal/cpu_cooling.c",
                                              "drivers/thermal/samsung/gpu_cooling.c",
                                              "drivers/thermal/samsung/exynos_tmu.c",
                                              "drivers/thermal/samsung/isp_cooling.c",
                                              "drivers/thermal/samsung/exynos_thermal_common.c",
                                              "drivers/thermal/samsung/cpu_cooling.c",
                                              "drivers/thermal/samsung/exynos_tmu_data.c",
                                              "drivers/trace/exynos-ss.c",
                                              "drivers/trace/exynos-coresight.c",
                                              "drivers/trace/exynos-coresight-etm.c",
                                              "drivers/trace/exynos8890-busmon.c",
                                              "drivers/tty/serial/samsung.c",
                                              "drivers/usb/phy/phy-ab8500-usb.c",
                                              "drivers/usb/class/cdc-acm.c",
                                              "drivers/usb/storage/sddr09.c",
                                              "drivers/usb/notify/host_notify_class.c",
                                              "drivers/usb/notify/usblog_proc_notify.c",
                                              "drivers/usb/notify/usb_notifier.c",
                                              "drivers/usb/notify/usb_notify.c",
                                              "drivers/usb/notify/dock_notify.c",
                                              "drivers/usb/notify/usb_notify_sysfs.c",
                                              "drivers/usb/notify/external_notify.c",
                                              "drivers/usb/serial/visor.c",
                                              "drivers/usb/serial/option.c",
                                              "drivers/usb/serial/pl2303.c",
                                              "drivers/usb/serial/qcaux.c",
                                              "drivers/usb/serial/ipaq.c",
                                              "drivers/usb/serial/qcserial.c",
                                              "drivers/usb/dwc2/gadget.c",
                                              "drivers/usb/dwc3/dwc3-exynos.c",
                                              "drivers/usb/dwc3/otg.c",
                                              "drivers/usb/dwc3/ep0.c",
                                              "drivers/usb/dwc3/gadget.c",
                                              "drivers/usb/gadget/udc/omap_udc.c",
                                              "drivers/usb/gadget/udc/s3c-hsudc.c",
                                              "drivers/usb/gadget/udc/s3c2410_udc.c",
                                              "drivers/usb/gadget/android.c",
                                              "drivers/usb/gadget/function/storage_common.c",
                                              "drivers/usb/gadget/function/serial_acm.c",
                                              "drivers/usb/gadget/function/f_mass_storage.c",
                                              "drivers/usb/gadget/function/f_fs.c",
                                              "drivers/usb/gadget/function/f_diag.c",
                                              "drivers/usb/gadget/function/f_ncm.c",
                                              "drivers/usb/gadget/function/u_ncm.c",
                                              "drivers/usb/gadget/function/f_acm.c",
                                              "drivers/usb/gadget/function/f_dm.c",
                                              "drivers/usb/gadget/function/multi_config.c",
                                              "drivers/usb/gadget/function/u_ether.c",
                                              "drivers/usb/gadget/function/f_rndis.c",
                                              "drivers/usb/gadget/function/f_mtp_samsung.c",
                                              "drivers/usb/gadget/function/f_sdb.c",
                                              "drivers/usb/gadget/function/u_composite_notifier.c",
                                              "drivers/usb/gadget/legacy/mass_storage.c",
                                              "drivers/usb/gadget/legacy/g_ffs.c",
                                              "drivers/usb/gadget/legacy/multi.c",
                                              "drivers/usb/gadget/u_f.c",
                                              "drivers/usb/gadget/composite.c",
                                              "drivers/usb/core/quirks.c",
                                              "drivers/usb/host/ohci-s3c2410.c",
                                              "drivers/usb/host/ehci-exynos.c",
                                              "drivers/usb/host/ohci-exynos.c",
                                              "drivers/video/fbdev/s3c-fb.c",
                                              "drivers/video/fbdev/au1200fb.c",
                                              "drivers/video/fbdev/aty/radeon_pm.c",
                                              "drivers/video/fbdev/exynos/exynos_mipi_dsi_common.c",
                                              "drivers/video/fbdev/exynos/exynos_mipi_dsi.c",
                                              "drivers/video/fbdev/exynos/decon_8890/decon_s.c",
                                              "drivers/video/fbdev/exynos/decon_8890/panels/dsim_panel.c",
                                              "drivers/video/fbdev/exynos/decon_8890/panels/s6e3hf4_s6e3ha5_wqhd_lcd_ctrl.c",
                                              "drivers/video/fbdev/exynos/decon_8890/panels/lcd_sysfs.c",
                                              "drivers/video/fbdev/exynos/decon_8890/panels/mdnie_tuning.c",
                                              "drivers/video/fbdev/exynos/decon_8890/panels/s6e3ha3_s6e3ha2_wqhd_lcd_ctrl.c",
                                              "drivers/video/fbdev/exynos/decon_8890/panels/mdnie_lite.c",
                                              "drivers/video/fbdev/exynos/decon_8890/panels/dimming_core_hero.c",
                                              "drivers/video/fbdev/exynos/decon_8890/panels/s6e3hf2_wqxga_lcd_ctrl.c",
                                              "drivers/video/fbdev/exynos/decon_8890/panels/dimming_core.c",
                                              "drivers/video/fbdev/exynos/decon_8890/panels/dsim_backlight_s6e3fa2.c",
                                              "drivers/video/fbdev/exynos/decon_8890/panels/dsim_backlight_grace.c",
                                              "drivers/video/fbdev/exynos/decon_8890/panels/dsim_backlight_u.c",
                                              "drivers/video/fbdev/exynos/decon_8890/panels/s6e3fa2_fhd_lcd_ctrl.c",
                                              "drivers/video/fbdev/exynos/decon_8890/panels/ddi_spi.c",
                                              "drivers/video/fbdev/exynos/decon_8890/panels/dsim_backlight_dynamic.c",
                                              "drivers/video/fbdev/exynos/decon_8890/vpp/vpp_reg_8890.c",
                                              "drivers/video/fbdev/exynos/decon_8890/vpp/vpp_drv.c",
                                              "drivers/video/fbdev/exynos/decon_8890/decon_core.c",
                                              "drivers/video/fbdev/exynos/decon_8890/decon_f.c",
                                              "drivers/video/fbdev/exynos/decon_8890/decon_helper.c",
                                              "drivers/video/fbdev/exynos/decon_8890/dsim_reg_8890.c",
                                              "drivers/video/fbdev/exynos/decon_8890/decon_t.c",
                                              "drivers/video/fbdev/exynos/decon_8890/decon_reg_8890.c",
                                              "drivers/video/fbdev/exynos/decon_8890/decon_bts.c",
                                              "drivers/video/fbdev/exynos/decon_8890/dsim_drv.c",
                                              "drivers/video/fbdev/exynos/decon_8890/decon_dsi.c",
                                              "drivers/video/fbdev/exynos/exynos_mipi_dsi_lowlevel.c",
                                              "drivers/video/fbdev/exynos/s6e8ax0.c",
                                              "drivers/video/backlight/lms501kf03.c",
                                              "drivers/video/backlight/ld9040.c",
                                              "drivers/video/backlight/lms283gf05.c",
                                              "drivers/video/backlight/ltv350qv.c",
                                              "drivers/video/backlight/s6e63m0.c",
                                              "drivers/video/backlight/ams369fg06.c",
                                              "drivers/watchdog/s3c2410_wdt.c",
                                              "sound/soc/samsung",
                                              NULL};

void print_err(char *prog_name) {
    std::cerr << "[!] This program identifies all the required vendor bitcode files for each bitcode file";
    std::cerr << "[!] and saves then in llvm_link_out folder in the same directory as bitcode file.";
    std::cerr << "\n[!] After this run: link_llvm_bc_file.py to link and get a complete analyzable file.\n";
    std::cerr << "[?] " << prog_name << " <llvm_bin_output> <1(mediatek)|2(qualcomm)|3(huawei)|4(samsung)>\n";
    exit(-1);
}

bool str_ends_with(const string& a, const string& b) {
    if (b.size() > a.size()) return false;
    return std::equal(a.begin() + a.size() - b.size(), a.end(), b.begin());
}

int is_regular_file(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    if(S_ISREG(path_stat.st_mode)) {
        std::string path_str(path);
        std::string bc_suffix(".llvm.bc");
        return str_ends_with(path_str, bc_suffix);
    }
    return false;
}

int is_dir(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}

int is_interesting_folder(char *curr_folder, unsigned long arch_no) {
    unsigned long array_size = 0;
    const char **inter_folders;
    unsigned  long i;
    const char *curr_fl;
    char to_check_file[1024];
    switch(arch_no) {
        case 1:
            inter_folders = mediatek_driver_folder;
            array_size = SIZEOF_ARRAY(mediatek_driver_folder);
            break;
        case 2:
            inter_folders = qualcomm_driver_folders;
            array_size = SIZEOF_ARRAY(qualcomm_driver_folders);
            break;
        case 3:
            inter_folders = huawei_driver_folders;
            array_size = SIZEOF_ARRAY(huawei_driver_folders);
            break;
        case 4:
            inter_folders = samsung_driver_folders;
            array_size = SIZEOF_ARRAY(samsung_driver_folders);
            break;
        default:
            std::cerr << "Invalid arch number, Valid arch numbers are: 1(mediatek)|2(qualcomm)|3(huawei)|4(samsung)\n";
            exit(-2);
    }

    for(i=0; i<array_size; i++) {
        curr_fl = inter_folders[i];

        if(curr_fl != NULL) {
            strcpy(to_check_file, curr_fl);
            if(strcmp(to_check_file,"sound/soc/samsung")) {
                to_check_file[strlen(to_check_file) - 2] = 0;
            }
            //std::cout << "Checking against:" << curr_fl << ":" << strlen(curr_fl) << "\n";
            if (std::strstr(curr_folder, to_check_file)) {
                std::cout << "[+] Interesting Folder:" << curr_folder << ": Matched with:" << curr_fl << "\n";
                return 1;
            }
            if(!strcmp(curr_fl,"sound/soc/samsung")) {
                break;
            }
        }
    }
    return 0;

}

void recursively_traverse_dir(char *dir_path, std::vector<std::string> &all_files) {
    char total_file_len[2048];
    char *child_file_ptr;
    struct dirent *entry;
    DIR *dp;

    dp = opendir(dir_path);
    assert(strlen(dir_path) < (sizeof(total_file_len) - 258));

    strcpy(total_file_len, dir_path);
    child_file_ptr = total_file_len + strlen(total_file_len);
    while ((entry = readdir(dp))) {
        // Ignore if some idiot already ran driver linker before.
        // save the idiot.
        if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..") ||
                !strcmp(entry->d_name, DR_LINK_OUT)) {
            continue;
        }
        strcpy(child_file_ptr, "/");
        strcat(child_file_ptr, entry->d_name);
        if(is_regular_file(total_file_len)) {
            std::string toadd_file(total_file_len);
            all_files.push_back(toadd_file);
        } else if(is_dir(total_file_len)) {
            recursively_traverse_dir(total_file_len, all_files);
        }
    }
    closedir(dp);
}

void get_all_interesting_files(char *dir_path, unsigned long arch_no,
                               std::vector<std::string> &dst_interesting_files,
                                std::vector<std::string> &interesting_folders) {
    char total_file_len[2048];
    char *child_file_ptr;
    struct dirent *entry;
    DIR *dp;

    dp = opendir(dir_path);
    assert(strlen(dir_path) < (sizeof(total_file_len) - 258));

    strcpy(total_file_len, dir_path);
    child_file_ptr = total_file_len + strlen(total_file_len);
    while ((entry = readdir(dp))) {
        // Ignore if some idiot already ran driver linker before.
        // save the idiot.
        if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..") ||
                !strcmp(entry->d_name, DR_LINK_OUT)) {
            continue;
        }
        strcpy(child_file_ptr, "/");
        strcat(child_file_ptr, entry->d_name);
        if(is_dir(total_file_len)) {
            if(is_interesting_folder(total_file_len, arch_no)) {
                std::string toadd_file(total_file_len);
                interesting_folders.push_back(toadd_file);
                recursively_traverse_dir(total_file_len, dst_interesting_files);
            } else {
                get_all_interesting_files(total_file_len, arch_no, dst_interesting_files, interesting_folders);
            }
        } else if(is_regular_file(total_file_len)) {
            if(is_interesting_folder(total_file_len, arch_no)) {
                std::string toadd_file(total_file_len);
                dst_interesting_files.push_back(toadd_file);
            }
        }
    }
    closedir(dp);
}

int main(int argc, char *argv[]) {
    //check args
    if(argc < 3) {
        print_err(argv[0]);
    }

    char *src_bin_out = argv[1];
    char *dummy_ptr;
    unsigned long arch_no, i;
    arch_no = strtol(argv[2], &dummy_ptr, 10);

    std::vector<std::string> all_interesting_files;
    std::vector<std::string> interesting_base_folder;
    get_all_interesting_files(src_bin_out, arch_no, all_interesting_files, interesting_base_folder);
    std::cout << "[+] Found:" << all_interesting_files.size() << " Interesting bitcode files\n";
    std::cout << "[*] Processing bitcode files\n";

    std::vector<BitCodeFunctions*> allbitcode_files;
    LLVMContext context;

    for(i=0; i<all_interesting_files.size(); i++) {
        std::string curr_bc_file = all_interesting_files[i];
        ErrorOr<std::unique_ptr<MemoryBuffer>> fileOrErr = MemoryBuffer::getFileOrSTDIN(curr_bc_file);
        const char *fileBuf = fileOrErr.get()->getMemBufferRef().getBufferStart();
        if(fileBuf[0] != 'B' || fileBuf[1] != 'C') {
            continue;
        }
        if (std::error_code ec = fileOrErr.getError())
        {
            std::cerr << "[-] Error opening input file: " + ec.message() << std::endl;
            return 2;
        }
        std::cout << "[*] " << " Processing " << "(" << (i+1) << " of " << all_interesting_files.size() << "): "
                  << curr_bc_file << "\n";
        ErrorOr<std::unique_ptr<llvm::Module>> moduleOrErr = parseBitcodeFile(fileOrErr.get()->getMemBufferRef(), context);
        std::cout << "[*] " << " Processed " << "(" << (i+1) << " of " << all_interesting_files.size() << "): "
                  << curr_bc_file << "\n";
        if (std::error_code ec = moduleOrErr.getError())
        {
            std::cerr << "[-] Error reading Module: " + ec.message() << std::endl;
            return 3;
        }
        if (moduleOrErr.get().get() == nullptr)
        {
            std::cerr << "[-] Error reading Module" << std::endl;
            return 3;
        }
        Module *m = moduleOrErr.get().get();
        std::vector<string> definedFunctions;
        std::vector<string> declaredFunctions;
        definedFunctions.clear();
        declaredFunctions.clear();

        for (auto iter1 = m->getFunctionList().begin(); iter1 != m->getFunctionList().end(); iter1++) {
            Function &f = *iter1;
            // This means function is defined.
            if (!f.isDeclaration()) {
                definedFunctions.push_back(f.getName().str());
            } else {
                declaredFunctions.push_back(f.getName().str());
            }
        }

        allbitcode_files.push_back(new BitCodeFunctions(curr_bc_file.c_str(), definedFunctions, declaredFunctions));

    }

    std::cout << "[+] Processed all bitcode files.\n";
    std::cout << "[*] Trying to find dependency bc files\n";
    for(auto srcBcFile:allbitcode_files) {
        char link_output_folder[2048];
        char to_run_command[4096];
        strcpy(link_output_folder, srcBcFile->target_bc_file.c_str());
        char *last_slash = strrchr(link_output_folder, '/');
        last_slash++;
        strcpy(last_slash, DR_LINK_OUT);
        strcpy(to_run_command, "mkdir -p ");
        strcat(to_run_command, link_output_folder);
        system(to_run_command);
        std::cout << "[*] Trying to find dependent bitcode files for:" << srcBcFile->target_bc_file << "\n";
        std::cout << "[$] All Dependent files will be present at:" << link_output_folder << "\n";
        for(auto dstBcFile: allbitcode_files) {
            if(srcBcFile->isNeeded(dstBcFile)) {
                std::cout<< "  [+] Needed BC File:" << dstBcFile->target_bc_file << "\n";
                strcpy(to_run_command, "cp ");
                strcat(to_run_command, dstBcFile->target_bc_file.c_str());
                strcat(to_run_command, " ");
                strcat(to_run_command, link_output_folder);
                system(to_run_command);
            }
        }
    }
}

