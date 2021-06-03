Ratchet : UGen {

    *ar { |input=0.0, active=0, length=0.1, rate=1.0|
        ^this.multiNew('audio', input.asAudioRateInput, active, length, rate);
    }

    checkInputs {

        if(inputs[0].rate != \audio){
            ^"input is not audio rate!".throw;
        };

        ^this.checkValidInputs;
    }

}