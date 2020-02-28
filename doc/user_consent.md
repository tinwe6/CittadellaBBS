# User consent to process personal data in Cittadella/UX

When a user first registers to the BBS, he is shown two texts:

- `./server/lib/messaggi/privacy`
- `./server/lib/messaggi/data_protection`

The former describes the privacy policy of the BBS, the latter specifies what use will be made of his personal data, his rights to have the data deleted, and include contact details of the management of the BBS.

The user is then asked whether he accepts the terms. If he accepts them, he can proceed with registration and enjoy the bbs. Otherwise the registration is interrupted and the user is asked to login anonymously as a guest or leave the BBS.


## Updating the terms

When the privacy and data protection policy of the BBS is changed (by editing the above mentioned `privacy` and `data_protection` texts), the system operators should request that the users confirm their consent to the terms.
They do that from within the client, using the command **`.Sd`**:

`<.><S>ysop reset <d>ata protection consent`

Then, the next time the users log into the BBS, they will be presented the new conditions and asked to renew their consent.


## What happens if the user does not renew the consent?

If a user disagrees with the new terms and decides not to renew the consent, the decision is saved in his user record and the system operators are automatically notified via post in the `Sysop Room>`. The system operators should then take action to respect the wishes of the user, deleting the user's account and private data within 48 hours.

These 48 hours are a cool-down period during which the user can backtrack on his decision by logging into the BBS, at which point the request for renewal of consent is presented again. If consent is given, the access to the BBS is restored and the request for account and data deletion is revoked (and notified via post in the `Sysop Room>`). 


## How can a user revoke the terms?

The procedure should be described in the `data_protection` file. For example, the user might be asked to send a request for account and personal data deletion to the system operators via an internal `Mail>` or to the BBS' official email address.


## Other Sysop tools

The system operators can check the list of users that are _not registered_, users that have _not given consent_ to process their personal data or need to _renew their consent_, and users that have _requested the deletion_ of their account and personal data, using the client command `.Sru`:

`<.><S>ysop <r>ead <u>nregistered users`

It is advised that the system operators use this command to check the user's will before they take action and delete irreversibly the account and associated data.


## Internal Mechanism
The following flags are stored in the user data `dati_ut.sflags[0]`

* `SUT_CONSENT` - set iff the user accepted the terms last time they were presented to her
* `SUT_NEED_CONSENT` - if set the user will be asked to renew her consent the next time she logs in 
* `SUT_DELETEME` - the user has requested the deletion of her account and related personal data

If `SUT_CONSENT` is not set _or_ `SUT_NEED_CONSENT` is set, the user is asked to accept the terms at login time.
The server _refuses_ to complete the login procedure unless `SUT_CONSENT` is set and `SUT_NEED_CONSENT` cleared.

The server accepts the following commands:

* `"GCST x"` (Give ConSenT) where `x` is 1 if the user gave its consent, 0 otherwise. It sets the `SUT_CONSENT` bit to`x` and clears the `SUT_NEED_CONSENT` for the user that sent the command. Additionally, if `x` is 0, it sets the `SUT_DELETEME` flag of the user. Instead, if `x` is 1 and the `SUT_DELETEME` flag was set, it clears it. In both cases the server notifies the system operators of the user's decision by sending a `citta_post()` to the `Sysop Room>`.

* `"RQST"` (Request ConSenT - Sysop only). It sets the `SUT_NEED_CONSENT` bit for all the users.

* `"UUSR unreg|no_consent|deleteme"` (Unregistered USeRs - Sysop only) - Sends the list of users satifying:                                             
	- `unreg`: if true, include all users that have not completed their registration successfully,
	- `no_consent`: if true, include all users that have not yet accepted the terms,
	- `deleteme`: if true, include all users that have requested the deletion of their account.

	Returns a list, each line corresponding to a user,                      
    - `"SEGUE_LISTA"`                                                        
    - `"OK name|calls|numposts|lastcall|level|registered|consent|renew|delete"`
	- `...`         
 	- `"000"`

 	Here, `registered` is true if the user completed the registration procedure by providing her personal data,
	`consent` is true if the user gave her consent at least once and never revoked it,
	`renew` is true if the user must renew her consent, and `delete` is true if the user asked the deletion of
	her account and personal data.
 
**Note**: When the server receives the command `USR1`, sent by the client to authenticate the user with a password, it checks whether the user needs to give or renew her consent. If so, the connection is enters the `CON_CONSENT` state that can be exited only through the `GCST` command with a positive consent.
 
