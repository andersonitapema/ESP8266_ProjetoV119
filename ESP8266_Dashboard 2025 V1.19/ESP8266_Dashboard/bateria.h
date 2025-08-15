#ifndef BATERIA_H
#define BATERIA_H

class BatteryMonitor {
private:
    const float _minVoltage = 3.0f;   // 0%
    const float _maxVoltage = 4.2f;   // 100%
    const float _adcReference = 6.0f; // Valor máximo do ADC (6V)
    float _readings[5] = {0};
    int _index = 0;

public:
    // Membros públicos (agora documentados)
    float voltage = 0.0f;      // Tensão atual em Volts
    float percentage = 0.0f;   // Porcentagem (0-100%)

    void update() {
        // 1. Nova leitura
        float raw = analogRead(A0) * (_adcReference / 1023.0f);
        
        // 2. Filtro de média móvel
        _readings[_index] = raw;
        _index = (_index + 1) % 5;
        
        // 3. Cálculo da média
        voltage = 0;
        for(int i = 0; i < 5; i++) {
            voltage += _readings[i];
        }
        voltage /= 5;

        // 4. Validação
        if(voltage > _adcReference || voltage < 0) {
            voltage = 0;
            percentage = 0;
            return;
        }

        // 5. Cálculo da porcentagem
        percentage = constrain(
            (voltage - _minVoltage) / (_maxVoltage - _minVoltage) * 100.0f,
            0.0f, 100.0f
        );
    }
};

extern BatteryMonitor battery; // Instância global

#endif