tmp<fv::convectionScheme<scalar>> mvConvection
(
    fv::convectionScheme<scalar>::New
    (
        mesh,
        fields,
        phi,
        mesh.divScheme("div(phi,Yi_h)")
    )
);
{
    radiation->correct();
    combustion->correct();

    HRR = combustion->Qdot();
   
    volScalarField Yt(0.0*Y[0]);

    forAll(Y, i)
    {
        if (i != inertIndex)
        {
            volScalarField& Yi = Y[i];
            
            fvScalarMatrix YiEqn
            (
                fvm::ddt(rho, Yi)
              + mvConvection->fvmDiv(phi, Yi)
              - fvm::laplacian(turbulence->alphaEff(), Yi)
              ==
                parcels.SYi(i, Yi)
            //   + surfaceFilm.Srho(i)
              + combustion->R(Yi)
            //   + fvOptions(rho, Yi)
            );
            YiEqn.relax();

            // fvOptions.constrain(YiEqn);

            YiEqn.solve("Yi");

            // fvOptions.correct(Yi);

            Yi.max(0.0);
            Yt += Yi;
        }
    }

    Y[inertIndex] = scalar(1) - Yt;
    Y[inertIndex].max(0.0);

    volScalarField& he = thermo.he();
    
    fvScalarMatrix EEqn
    (
        fvm::ddt(rho, he) + mvConvection->fvmDiv(phi, he)
      + fvc::ddt(rho, K) + fvc::div(phi, K)
      + (
            he.name() == "e"
          ? fvc::div
            (
                fvc::absolute(phi/fvc::interpolate(rho), U),
                p,
                "div(phiv,p)"
            )
          : -dpdt
        )
      - fvm::laplacian(turbulence->alphaEff(), he)
     ==
        // combustion->Qdot() // ha Eqn has no reaction source
        radiation->Sh(thermo, he)
      + parcels.Sh(he)
    //   + surfaceFilm.Sh()
    //   + fvOptions(rho, he)
    );

    EEqn.relax();

    // fvOptions.constrain(EEqn);

    EEqn.solve();

    // fvOptions.correct(he);
    
    // thermo.correct();
    chemistry->correctThermo();

    Info<< "min/max(T) = "
        << min(T).value() << ", " << max(T).value() << endl;
}
