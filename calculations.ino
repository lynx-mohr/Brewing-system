
/*Calculation of Strike temperature

Initial Infusion Temperature formula.W = (.2/R)(T2-T1)+T2
Where:
W = Strike water temperature °F
R = Water to grist ratio in quarts/lb (i.e. 1.25 or 1.5)
T1 = Temp. of your dry grain °F
T2 = Desired mash temp °F 
*/ 

//This function calculates the Strike temp according to the John Palmer formula

//val is in desired malt thickness in liter/kg
float strikeTempFunction(float mashThicknessFinal, float mashTempFinal, float grainTemp){ 

strikeTemp = (0.2 / mashThicknessFinal) * (mashTempFinal - grainTemp) + mashTempFinal; 

return strikeTemp;

}

//This function calculates how much water is needed for mashing

//val is alt thickness in liter/kg

float mashWaterVolFunction(float grainWeightFinal, float mashThicknessFinal){ 

 return mashThicknessFinal * grainWeightFinal;

}
