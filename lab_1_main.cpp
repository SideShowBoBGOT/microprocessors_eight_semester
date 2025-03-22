#include "stm32f103x6.h"

static void init_leds() {
    GPIOA->CRL |= GPIO_CRL_MODE0_0;
    GPIOA->CRL |= GPIO_CRL_MODE1_0;
    GPIOA->CRL |= GPIO_CRL_MODE2_0;
    GPIOA->CRL |= GPIO_CRL_MODE3_0;
    GPIOA->CRL |= GPIO_CRL_MODE4_0;
    GPIOA->CRL |= GPIO_CRL_MODE5_0;
    GPIOA->CRL |= GPIO_CRL_MODE6_0;
    GPIOA->CRH |= GPIO_CRH_MODE9_0;
    GPIOA->CRH |= GPIO_CRH_MODE10_0;
    GPIOA->CRH |= GPIO_CRH_MODE11_0;
}

static void power_0_4() {
    GPIOA->BSRR = GPIO_BSRR_BS0;
    GPIOA->BSRR = GPIO_BSRR_BS1;
    GPIOA->BSRR = GPIO_BSRR_BS2;
    GPIOA->BSRR = GPIO_BSRR_BS3;
    GPIOA->BSRR = GPIO_BSRR_BS4;
}

static void unpower_0_4() {
    GPIOA->BSRR = GPIO_BSRR_BR0;
    GPIOA->BSRR = GPIO_BSRR_BR1;
    GPIOA->BSRR = GPIO_BSRR_BR2;
    GPIOA->BSRR = GPIO_BSRR_BR3;
    GPIOA->BSRR = GPIO_BSRR_BR4;
}

static void power_5_9() {
    GPIOA->BSRR = GPIO_BSRR_BS5;
    GPIOA->BSRR = GPIO_BSRR_BS6;
    GPIOA->BSRR = GPIO_BSRR_BS9;
    GPIOA->BSRR = GPIO_BSRR_BS10;
    GPIOA->BSRR = GPIO_BSRR_BS11;
}

static void unpower_5_9() {
    GPIOA->BSRR = GPIO_BSRR_BR5;
    GPIOA->BSRR = GPIO_BSRR_BR6;
    GPIOA->BSRR = GPIO_BSRR_BR9;
    GPIOA->BSRR = GPIO_BSRR_BR10;
    GPIOA->BSRR = GPIO_BSRR_BR11;
}

extern "C" void TIM2_IRQHandler() {
    if (GPIOA->IDR & GPIO_IDR_IDR12) {
        unpower_5_9();

        if (GPIOA->ODR & GPIO_ODR_ODR0) {
            unpower_0_4();
        } else {
            power_0_4();
        }
    } else {
        unpower_0_4();

        if (GPIOA->ODR & GPIO_ODR_ODR5) {
            unpower_5_9();
        } else {
            power_5_9();
        }
    }
    
    TIM2->SR &= ~TIM_SR_UIF;
}

static constexpr auto MY_VARIANT_RCC_CFGR_PLLMULL = RCC_CFGR_PLLMULL3;
static constexpr uint8_t MY_VARIANT_N = 3;
static constexpr decltype(TIM_TypeDef::PSC) MY_VARIANT_PSC = MY_VARIANT_N * 1000;
static constexpr decltype(TIM_TypeDef::ARR) MY_VARIANT_ARR = 1000.0 / (static_cast<double>(MY_VARIANT_N) / 10.0);

static void rccSetup() {
    RCC->CR |= RCC_CR_HSION; // oscillator
    while (!(RCC->CR & RCC_CR_HSIRDY));

    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL)) | MY_VARIANT_RCC_CFGR_PLLMULL;

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    RCC->CFGR |= RCC_CFGR_SW_PLL; //PLL as system clock
    while (!(RCC->CFGR & RCC_CFGR_SWS_PLL));

    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_HPRE_DIV4;
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | FLASH_ACR_LATENCY_0;
}

int main() {
    rccSetup();
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN
                 |  RCC_APB2ENR_AFIOEN
                 |  RCC_APB2ENR_TIM1EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    init_leds();

    TIM2->PSC = MY_VARIANT_PSC - 1;
    TIM2->ARR = MY_VARIANT_ARR - 1;
    TIM2->DIER = TIM_DIER_UIE;
    NVIC_EnableIRQ(TIM2_IRQn);
    TIM2->CR1 = TIM_CR1_CEN;

    GPIOA->CRL &= ~(GPIO_CRL_MODE7 | GPIO_CRL_CNF7);
    GPIOA->CRL |= GPIO_CRL_MODE7_0 | GPIO_CRL_CNF7_1;
    GPIOA->CRH &= ~(GPIO_CRH_MODE8 | GPIO_CRH_CNF8);
    GPIOA->CRH |= GPIO_CRH_MODE8_0 | GPIO_CRH_CNF8_1;
    AFIO->MAPR |= AFIO_MAPR_TIM1_REMAP_0;

    TIM1->PSC = MY_VARIANT_PSC - 1;
    TIM1->ARR = 1000 - 1; 
    TIM1->CCR1 = 500 - 1;
    TIM1->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;

    // enable capture/compare, invert polarity and complementary output
    TIM1->CCER |= TIM_CCER_CC1E | TIM_CCER_CC1P | TIM_CCER_CC1NE;
    TIM1->BDTR |= TIM_BDTR_MOE; // output
    TIM1->CR1 |= TIM_CR1_CEN; // counter

    while (true) {}

    return 0;
}